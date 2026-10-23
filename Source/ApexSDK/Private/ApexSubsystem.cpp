// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#include "ApexSubsystem.h"

#include "ApexAuthorizationCodeSocket.h"
#include "ApexJson.h"
#include "ApexLauncherSubsystem.h"
#include "ApexSDKSettings.h"
#include "ApexUnrealHttp.h"

#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "IXRTrackingSystem.h"
#include "Interfaces/IPluginManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "SocketSubsystem.h"

#if PLATFORM_ANDROID
#include "Android/AndroidJNI.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogApex, Log, All);

namespace
{
const TCHAR* const kTokenArgument = TEXT("pixotoken");
const TCHAR* const kOptionalArgument = TEXT("optional");
const TCHAR* const kReturnTargetArgument = TEXT("returntarget");
const TCHAR* const kTargetTypeArgument = TEXT("targettype");
const TCHAR* const kPixoConfigFileName = TEXT("pixoconfig.cnf");
constexpr double kSerialNumberRetrySeconds = 5.0;
constexpr double kSerialNumberRefreshSeconds = 30.0;

/** Reads one of the core's "fill a buffer" values into an FString. */
template <typename TReadFunction>
FString ReadCoreString(TReadFunction readFunction)
{
	const size_t valueLength = readFunction(nullptr, 0);
	if (valueLength == 0)
	{
		return FString();
	}

	TArray<char> valueBuffer;
	valueBuffer.SetNumZeroed(static_cast<int32>(valueLength) + 1);
	readFunction(valueBuffer.GetData(), valueLength + 1);
	return FString(UTF8_TO_TCHAR(valueBuffer.GetData()));
}

TArray<uint8> ToUtf8Bytes(const FString& text)
{
	const FTCHARToUTF8 utf8Text(*text);
	TArray<uint8> textBytes;
	textBytes.Append(reinterpret_cast<const uint8*>(utf8Text.Get()), utf8Text.Length());
	return textBytes;
}

void SetStringOrNull(const TSharedRef<FJsonObject>& jsonObject, const FString& fieldName, const FString& fieldText)
{
	if (fieldText.IsEmpty())
	{
		jsonObject->SetField(fieldName, MakeShared<FJsonValueNull>());
		return;
	}

	jsonObject->SetStringField(fieldName, fieldText);
}

void SetObjectOrNull(const TSharedRef<FJsonObject>& jsonObject, const FString& fieldName,
					 const TSharedPtr<FJsonObject>& fieldObject)
{
	if (fieldObject.IsValid())
	{
		jsonObject->SetObjectField(fieldName, fieldObject);
		return;
	}

	jsonObject->SetField(fieldName, MakeShared<FJsonValueNull>());
}

/** Parses optional JSON object text: empty is valid and null, anything but an object is not. */
bool TryParseOptionalObject(const FString& jsonText, TSharedPtr<FJsonObject>& parsedObject)
{
	parsedObject.Reset();
	if (jsonText.TrimStartAndEnd().IsEmpty())
	{
		return true;
	}

	parsedObject = ApexJson::ParseObject(jsonText);
	return parsedObject.IsValid();
}
}  // namespace

void UApexSubsystem::FApexClientDeleter::operator()(ApexClient* client) const
{
	ApexClientDestroy(client);
}

UApexSubsystem::UApexSubsystem()
	: m_environment(EApexServerEnvironment::NA_PRODUCTION),
	  m_moduleId(-1),
	  m_sendXapiSessionEvents(true),
	  m_telemetrySink(EApexTelemetrySinkType::NONE),
	  m_nextSerialNumberCheckSeconds(0.0),
	  m_isLoggedIn(false),
	  m_sessionInProgress(false),
	  m_sessionId(0)
{
}

UApexSubsystem::~UApexSubsystem()
{
	DestroyClient();
}

UApexSubsystem* UApexSubsystem::Get(const UObject* worldContextObject)
{
	UWorld* world = GEngine != nullptr
						? GEngine->GetWorldFromContextObject(worldContextObject, EGetWorldErrorMode::ReturnNull)
						: nullptr;
	UGameInstance* gameInstance = world != nullptr ? world->GetGameInstance() : nullptr;
	return gameInstance != nullptr ? gameInstance->GetSubsystem<UApexSubsystem>() : nullptr;
}

void UApexSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
	Super::Initialize(collection);

#if PLATFORM_ANDROID
	// The core is linked into the game, so its JNI_OnLoad never runs; hand it the VM instead.
	ApexAndroidSetJavaVm(GJavaVM);
#endif

	const UApexSDKSettings* settings = GetDefault<UApexSDKSettings>();
	m_environment = settings->Environment;
	m_moduleId = settings->ModuleId;
	m_moduleName = settings->ModuleName;
	m_moduleVersion = settings->ModuleVersion;
	m_scenarioId = settings->DefaultScenarioId;
	m_sendXapiSessionEvents = settings->bSendXapiSessionEvents;
	if (!settings->IsModuleVersionValid())
	{
		UE_LOG(LogApex, Warning, TEXT("%s is not a valid module version (MAJOR.MINOR.PATCH)."), *m_moduleVersion);
	}

	ReadDeviceInformation();
	ReadPixoConfigFile();
	ReadLaunchArguments();
	CreateClient();

	UE_LOG(LogApex, Log, TEXT("Apex SDK %s on core %s (%s HTTP); module %d %s; %s."), *GetSdkVersion(),
		   *GetCoreVersion(), UTF8_TO_TCHAR(ApexBuiltinTransportName()), m_moduleId, *m_moduleVersion,
		   HasLaunchLoginToken() ? TEXT("a login token was passed") : TEXT("no login token was passed"));

	if (settings->bLoginWithLaunchTokenOnStart && HasLaunchLoginToken())
	{
		LoginWithToken(FString());
	}
}

void UApexSubsystem::Deinitialize()
{
	const UApexSDKSettings* settings = GetDefault<UApexSDKSettings>();
	if (m_sessionInProgress && settings->bCompleteSessionOnShutdown)
	{
		// Sent now; destroying the client waits up to a second for it.
		CompleteSession(FApexSessionResult(), FString());
	}

	TelemetryFlush();
	DestroyClient();
	Super::Deinitialize();
}

void UApexSubsystem::Tick(float /*deltaSeconds*/)
{
	const double nowSeconds = FPlatformTime::Seconds();
	PollDeviceSerialNumber(nowSeconds);
	if (m_client.IsValid())
	{
		ApexClientPoll(m_client.Get(), nowSeconds);
	}
}

TStatId UApexSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UApexSubsystem, STATGROUP_Tickables);
}

bool UApexSubsystem::IsTickable() const
{
	return !IsTemplate() && m_client.IsValid();
}

bool UApexSubsystem::IsTickableWhenPaused() const
{
	return true;
}

bool UApexSubsystem::IsTickableInEditor() const
{
	return false;
}

// ---------------------------------------------------------------- Core and environment

bool UApexSubsystem::IsCoreAvailable() const
{
	return m_client.IsValid();
}

FString UApexSubsystem::GetCoreVersion()
{
	return UTF8_TO_TCHAR(ApexVersionString());
}

FString UApexSubsystem::GetSdkVersion()
{
	TSharedPtr<IPlugin> plugin = IPluginManager::Get().FindPlugin(TEXT("ApexSDK"));
	return plugin.IsValid() ? plugin->GetDescriptor().VersionName : FString(TEXT("unknown"));
}

EApexServerEnvironment UApexSubsystem::GetEnvironment() const
{
	return m_environment;
}

void UApexSubsystem::ChangeEnvironment(EApexServerEnvironment newEnvironment)
{
	UE_LOG(LogApex, Log, TEXT("Changing environment to %d."), static_cast<int32>(newEnvironment));
	const bool wasLoggedIn = m_isLoggedIn;
	m_environment = newEnvironment;
	DestroyClient();
	CreateClient();
	if (wasLoggedIn)
	{
		OnLoggedOutNative.Broadcast();
		OnLoggedOut.Broadcast();
	}
}

FString UApexSubsystem::GetHeadsetApiUrl() const
{
	const UApexSDKSettings* settings = GetDefault<UApexSDKSettings>();
	if (!settings->HeadsetApiUrl.IsEmpty())
	{
		return settings->HeadsetApiUrl;
	}

	switch (m_environment)
	{
		case EApexServerEnvironment::NA_STAGING:
			return TEXT("https://modules.apex.stage.pixovr.com");
		case EApexServerEnvironment::NA_DEVELOPMENT:
			return TEXT("https://modules.apex.dev.pixovr.com");
		case EApexServerEnvironment::SA_PRODUCTION:
			return TEXT("https://saudi.modules.apex.pixovr.com");
		case EApexServerEnvironment::LOCAL:
			return TEXT("http://localhost:8001");
		default:
			return TEXT("https://modules.apex.pixovr.com");
	}
}

int32 UApexSubsystem::GetModuleId() const
{
	return m_moduleId;
}

FString UApexSubsystem::GetModuleName() const
{
	return m_moduleName;
}

FString UApexSubsystem::GetModuleVersion() const
{
	return m_moduleVersion;
}

FString UApexSubsystem::GetDeviceId() const
{
	return m_deviceId;
}

FString UApexSubsystem::GetDeviceModel() const
{
	return m_deviceModel;
}

FString UApexSubsystem::GetPlatformName() const
{
	return m_platformName;
}

FString UApexSubsystem::GetDeviceSerialNumber() const
{
	return m_deviceSerialNumber;
}

void UApexSubsystem::SetDeviceSerialNumber(const FString& serialNumber)
{
	if (serialNumber == m_deviceSerialNumber)
	{
		return;
	}

	m_deviceSerialNumber = serialNumber;
	if (m_client.IsValid())
	{
		ApexSetDeviceId(m_client.Get(), TCHAR_TO_UTF8(*m_deviceSerialNumber));
	}

	UE_LOG(LogApex, Log, TEXT("Device serial number set."));
}

void UApexSubsystem::CreateClient()
{
	const UApexSDKSettings* settings = GetDefault<UApexSDKSettings>();

	// The core copies every string before ApexClientCreate returns; these keep them alive until then.
	const FTCHARToUTF8 headsetApiUrl(*settings->HeadsetApiUrl);
	const FTCHARToUTF8 platformApiUrl(*settings->PlatformApiUrl);
	const FTCHARToUTF8 moduleVersion(*m_moduleVersion);
	const FTCHARToUTF8 deviceSerialNumber(*m_deviceSerialNumber);
	const FTCHARToUTF8 deviceModel(*m_deviceModel);
	const FTCHARToUTF8 platformName(*m_platformName);
	FString clientIpText;
	if (ISocketSubsystem* socketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM))
	{
		bool canBindAll = false;
		TSharedPtr<FInternetAddr> localAddress = socketSubsystem->GetLocalHostAddr(*GLog, canBindAll);
		clientIpText = localAddress.IsValid() ? localAddress->ToString(false) : FString();
	}

	const FTCHARToUTF8 clientIp(*clientIpText);
	const FTCHARToUTF8 telemetryUrl(*settings->TelemetryUrl);

	ApexConfig config;
	ApexConfigInit(&config);
	config.Environment = static_cast<int32_t>(m_environment);
	config.HeadsetApiUrl = settings->HeadsetApiUrl.IsEmpty() ? nullptr : headsetApiUrl.Get();
	config.PlatformApiUrl = settings->PlatformApiUrl.IsEmpty() ? nullptr : platformApiUrl.Get();
	config.ModuleId = m_moduleId;
	config.ModuleVersion = moduleVersion.Get();
	config.DeviceId = deviceSerialNumber.Get();
	config.DeviceModel = deviceModel.Get();
	config.Platform = platformName.Get();
	config.ClientIp = clientIpText.IsEmpty() ? nullptr : clientIp.Get();
	config.HeartbeatIntervalSeconds = settings->HeartbeatIntervalSeconds;
	config.RequestTimeoutMilliseconds =
		static_cast<uint32_t>(FMath::Max(1.0f, settings->RequestTimeoutSeconds) * 1000.0f);
	config.LogCallback = &UApexSubsystem::HandleLog;
	config.LogUserData = this;
	config.LogLevel = APEX_LOG_INFO;
	config.SessionFormat = m_sendXapiSessionEvents ? APEX_SESSION_FORMAT_XAPI : APEX_SESSION_FORMAT_DATA;

	m_telemetrySink = settings->IsPixoVRProviderEnabled() ? settings->TelemetrySink : EApexTelemetrySinkType::NONE;
	if (m_telemetrySink == EApexTelemetrySinkType::HTTP && settings->TelemetryUrl.IsEmpty())
	{
		UE_LOG(LogApex, Warning, TEXT("The HTTP telemetry sink needs a URL; logging packets instead."));
		m_telemetrySink = EApexTelemetrySinkType::LOG;
	}

	if (m_telemetrySink == EApexTelemetrySinkType::HTTP)
	{
		config.TelemetrySink = APEX_TELEMETRY_HTTP;
		config.TelemetryUrl = telemetryUrl.Get();
	}
	else if (m_telemetrySink != EApexTelemetrySinkType::NONE)
	{
		config.TelemetrySink = APEX_TELEMETRY_CALLBACK;
		config.TelemetryCallback = &UApexSubsystem::HandleTelemetryPacket;
		config.TelemetryUserData = this;
		m_telemetryDirectory = FPaths::ProjectSavedDir() / TEXT("ApexTelemetry");
	}

	config.TelemetryFlushIntervalSeconds = settings->TelemetryFlushIntervalSeconds;
	config.TelemetryPoseSamplesPerSecond = settings->PoseSamplesPerSecond;
	config.TelemetryMaximumRecordsPerPacket = settings->MaximumRecordsPerPacket;

	const bool hasBuiltinHttp = FCStringAnsi::Strcmp(ApexBuiltinTransportName(), "none") != 0;
	if (!hasBuiltinHttp || settings->bAlwaysUseUnrealHttp)
	{
		m_unrealHttp = MakeUnique<FApexUnrealHttp>();
		m_unrealHttp->Configure(config);
	}

	m_client.Reset(ApexClientCreate(&config));
	if (!m_client.IsValid())
	{
		UE_LOG(LogApex, Error, TEXT("The Apex core did not start; check the environment and its URLs."));
		m_unrealHttp.Reset();
		return;
	}

	if (m_unrealHttp.IsValid())
	{
		m_unrealHttp->SetClient(m_client.Get());
	}

	for (const TPair<FString, FString>& launchArgument : m_launchArguments)
	{
		ApexLaunchSet(m_client.Get(), TCHAR_TO_UTF8(*launchArgument.Key), TCHAR_TO_UTF8(*launchArgument.Value));
	}
}

void UApexSubsystem::DestroyClient()
{
	if (m_authorizationCodeSocket.IsValid())
	{
		m_authorizationCodeSocket->Close();
	}

	// No Unreal HTTP completion may reach the client after it is destroyed.
	if (m_unrealHttp.IsValid())
	{
		m_unrealHttp->Shutdown();
	}

	m_client.Reset();
	m_unrealHttp.Reset();
	m_pendingCalls.Empty();
	m_currentUser = FApexUser();
	m_isLoggedIn = false;
	m_moduleAccess = FApexModuleAccess();
	m_sessionInProgress = false;
	m_sessionId = 0;
}

void UApexSubsystem::ReadDeviceInformation()
{
	m_deviceId = FPlatformMisc::GetLoginId();
	m_deviceModel = FPlatformMisc::GetDeviceMakeAndModel();
	m_platformName = UGameplayStatics::GetPlatformName();
	if (GEngine != nullptr && GEngine->XRSystem.IsValid())
	{
		const FString xrSystemName = GEngine->XRSystem->GetSystemName().ToString();
		m_platformName = xrSystemName;
		if (m_deviceModel.IsEmpty())
		{
			m_deviceModel = xrSystemName;
		}
	}

	if (UApexLauncherSubsystem* launcher = UApexLauncherSubsystem::Get())
	{
		m_deviceSerialNumber = launcher->GetDeviceSerialNumber();
	}
}

void UApexSubsystem::ReadPixoConfigFile()
{
#if PLATFORM_ANDROID
	UApexLauncherSubsystem* launcher = UApexLauncherSubsystem::Get();
	if (!GetDefault<UApexSDKSettings>()->bReadPixoConfigFile || launcher == nullptr ||
		!launcher->DoesSharedFileExist(kPixoConfigFileName))
	{
		return;
	}

	const FString platformText =
		ApexJson::GetString(ApexJson::ParseObject(launcher->ReadSharedFile(kPixoConfigFileName)), TEXT("Platform"));
	if (platformText.IsEmpty())
	{
		UE_LOG(LogApex, Warning, TEXT("Failed to read pixoconfig.cnf."));
		return;
	}

	// As in the Unity SDK: "NA Production", "NA Stage", "NA Dev" or "SA".
	if (platformText.Contains(TEXT("NA")))
	{
		if (platformText.Contains(TEXT("Production")))
		{
			m_environment = EApexServerEnvironment::NA_PRODUCTION;
		}

		if (platformText.Contains(TEXT("Dev")))
		{
			m_environment = EApexServerEnvironment::NA_DEVELOPMENT;
		}

		if (platformText.Contains(TEXT("Stage")))
		{
			m_environment = EApexServerEnvironment::NA_STAGING;
		}
	}
	else if (platformText.Contains(TEXT("SA")))
	{
		m_environment = EApexServerEnvironment::SA_PRODUCTION;
	}

	UE_LOG(LogApex, Log, TEXT("pixoconfig.cnf selects environment %d."), static_cast<int32>(m_environment));
#endif
}

void UApexSubsystem::ReadLaunchArguments()
{
	TArray<FString> launchUrls;
#if PLATFORM_ANDROID
	if (UApexLauncherSubsystem* launcher = UApexLauncherSubsystem::Get())
	{
		launchUrls.Add(launcher->GetIntentData());
		for (const TCHAR* extraKey : {kTokenArgument, kOptionalArgument, kReturnTargetArgument, kTargetTypeArgument})
		{
			const FString extraValue = launcher->GetApplicationExtra(extraKey);
			if (!extraValue.IsEmpty())
			{
				ApplyLaunchArgument(extraKey, extraValue);
			}
		}
	}
#else
	// "-pixotoken t", "-pixotoken=t", and launcher URLs (pixovr://...?pixotoken=t), among Unreal's own flags.
	TArray<FString> commandLineTokens;
	const TCHAR* commandLineCursor = FCommandLine::Get();
	FString commandLineToken;
	while (FParse::Token(commandLineCursor, commandLineToken, false))
	{
		commandLineTokens.Add(commandLineToken);
	}

	for (int32 tokenIndex = 0; tokenIndex < commandLineTokens.Num(); ++tokenIndex)
	{
		const FString& token = commandLineTokens[tokenIndex];
		if (token.Contains(TEXT("://")))
		{
			launchUrls.Add(token);
			continue;
		}

		if (!token.StartsWith(TEXT("-")))
		{
			continue;
		}

		FString argumentKey = token.Mid(1);
		FString argumentValue;
		if (argumentKey.Split(TEXT("="), &argumentKey, &argumentValue))
		{
			ApplyLaunchArgument(argumentKey, argumentValue.TrimQuotes());
			continue;
		}

		const bool hasValue =
			commandLineTokens.IsValidIndex(tokenIndex + 1) && !commandLineTokens[tokenIndex + 1].StartsWith(TEXT("-"));
		if (hasValue)
		{
			ApplyLaunchArgument(argumentKey, commandLineTokens[tokenIndex + 1]);
			++tokenIndex;
		}
	}
#endif

	// The core decodes launcher URLs; a scratch client reads the arguments back out of them.
	ApexConfig scratchConfig;
	ApexConfigInit(&scratchConfig);
	TUniquePtr<ApexClient, FApexClientDeleter> scratchClient(ApexClientCreate(&scratchConfig));
	for (const FString& launchUrl : launchUrls)
	{
		if (launchUrl.IsEmpty() || !scratchClient.IsValid())
		{
			continue;
		}

		ApexLaunchParseUrl(scratchClient.Get(), TCHAR_TO_UTF8(*launchUrl));
		for (const TCHAR* argumentKey : {kTokenArgument, kOptionalArgument, kReturnTargetArgument, kTargetTypeArgument})
		{
			const FTCHARToUTF8 utf8Key(argumentKey);
			const FString argumentValue = ReadCoreString([&scratchClient, &utf8Key](char* buffer, size_t bufferSize)
			{
				return ApexLaunchGet(scratchClient.Get(), utf8Key.Get(), buffer, bufferSize);
			});
			if (!argumentValue.IsEmpty())
			{
				ApplyLaunchArgument(argumentKey, argumentValue);
			}
		}
	}
}

void UApexSubsystem::ApplyLaunchArgument(const FString& key, const FString& value)
{
	// A token already passed is kept, as in the Unity SDK.
	if (key == kTokenArgument && !GetLaunchArgument(kTokenArgument).IsEmpty())
	{
		return;
	}

	SetLaunchArgument(key, value);
}

void UApexSubsystem::PollDeviceSerialNumber(double nowSeconds)
{
#if PLATFORM_ANDROID
	// ManageXR reports the serial number some time after startup, and again when it changes.
	if (nowSeconds < m_nextSerialNumberCheckSeconds)
	{
		return;
	}

	if (UApexLauncherSubsystem* launcher = UApexLauncherSubsystem::Get())
	{
		const FString serialNumber = launcher->GetDeviceSerialNumber();
		if (!serialNumber.IsEmpty())
		{
			SetDeviceSerialNumber(serialNumber);
		}
	}

	m_nextSerialNumberCheckSeconds =
		nowSeconds + (m_deviceSerialNumber.IsEmpty() ? kSerialNumberRetrySeconds : kSerialNumberRefreshSeconds);
#endif
}

// ---------------------------------------------------------------- Calls

bool UApexSubsystem::TrackCall(ApexCallId callId, FPayloadCallback onResult)
{
	if (callId == 0)
	{
		Fail(onResult, EApexErrorCode::INTERNAL, TEXT("The core did not start the call."));
		return false;
	}

	m_pendingCalls.Add(callId, MoveTemp(onResult));
	return true;
}

bool UApexSubsystem::RequireClient(const FPayloadCallback& onResult)
{
	if (m_client.IsValid())
	{
		return true;
	}

	Fail(onResult, EApexErrorCode::INVALID_STATE, TEXT("The Apex core is not running."));
	return false;
}

void APEX_CALL UApexSubsystem::HandleResult(void* userData, const ApexResult* result)
{
	UApexSubsystem* subsystem = static_cast<UApexSubsystem*>(userData);
	if (subsystem == nullptr || result == nullptr)
	{
		return;
	}

	FPayloadCallback onResult;
	if (!subsystem->m_pendingCalls.RemoveAndCopyValue(result->CallId, onResult))
	{
		return;
	}

	if (onResult)
	{
		onResult(MakeResponse(*result), result->Json != nullptr ? FString(UTF8_TO_TCHAR(result->Json)) : FString());
	}
}

void APEX_CALL UApexSubsystem::HandleLog(void* /*userData*/, int32_t logLevel, const char* message)
{
	const FString messageText = UTF8_TO_TCHAR(message != nullptr ? message : "");
	switch (logLevel)
	{
		case APEX_LOG_ERROR:
			UE_LOG(LogApex, Error, TEXT("core: %s"), *messageText);
			break;
		case APEX_LOG_WARNING:
			UE_LOG(LogApex, Warning, TEXT("core: %s"), *messageText);
			break;
		case APEX_LOG_INFO:
			UE_LOG(LogApex, Log, TEXT("core: %s"), *messageText);
			break;
		default:
			UE_LOG(LogApex, Verbose, TEXT("core: %s"), *messageText);
			break;
	}
}

void APEX_CALL UApexSubsystem::HandleTelemetryPacket(void* userData, const char* packetJson)
{
	UApexSubsystem* subsystem = static_cast<UApexSubsystem*>(userData);
	if (subsystem == nullptr || packetJson == nullptr)
	{
		return;
	}

	const FString packetText = UTF8_TO_TCHAR(packetJson);
	if (subsystem->m_telemetrySink == EApexTelemetrySinkType::LOG)
	{
		UE_LOG(LogApex, Log, TEXT("telemetry: %s"), *packetText);
		return;
	}

	// FILE: <session>-<sequence>.json, as the Unity SDK names them.
	const TSharedPtr<FJsonObject> packetObject = ApexJson::ParseObject(packetText);
	const FString registration = ApexJson::GetString(packetObject, TEXT("r"));
	const int64 sequenceNumber = ApexJson::GetInteger(packetObject, TEXT("seq"));
	const FString fileName =
		FString::Printf(TEXT("%s-%05lld.json"), registration.IsEmpty() ? TEXT("nosession") : *registration,
						static_cast<long long>(sequenceNumber));
	const FString filePath = subsystem->m_telemetryDirectory / fileName;
	if (!FFileHelper::SaveStringToFile(packetText, *filePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(LogApex, Warning, TEXT("Failed to write telemetry to %s."), *filePath);
	}
}

FApexResponse UApexSubsystem::MakeResponse(const ApexResult& result)
{
	FApexResponse response;
	response.Error = static_cast<EApexErrorCode>(FMath::Clamp<int32>(result.Error, 0, APEX_ERR_INTERNAL));
	response.HttpStatus = result.HttpStatus;
	response.Message = UTF8_TO_TCHAR(result.Message != nullptr ? result.Message : "");
	return response;
}

void UApexSubsystem::Fail(const FPayloadCallback& onResult, EApexErrorCode error, const FString& message)
{
	UE_LOG(LogApex, Warning, TEXT("%s"), *message);
	if (onResult)
	{
		onResult(FApexResponse::MakeFailure(error, message), FString());
	}
}

// ---------------------------------------------------------------- Login

bool UApexSubsystem::IsLoggedIn() const
{
	return m_isLoggedIn;
}

FApexUser UApexSubsystem::GetCurrentUser() const
{
	return m_currentUser;
}

bool UApexSubsystem::HasLaunchLoginToken() const
{
	return !GetLaunchArgument(kTokenArgument).IsEmpty();
}

void UApexSubsystem::Ping(FResultCallback onResult)
{
	FPayloadCallback onPayload = [onResult](const FApexResponse& response, const FString& /*payloadJson*/)
	{
		if (onResult)
		{
			onResult(response);
		}
	};
	if (RequireClient(onPayload))
	{
		TrackCall(ApexPing(m_client.Get(), &UApexSubsystem::HandleResult, this), MoveTemp(onPayload));
	}
}

void UApexSubsystem::Login(const FString& username, const FString& password, FUserCallback onResult)
{
	FPayloadCallback onPayload = [this, onResult](const FApexResponse& response, const FString& payloadJson)
	{
		HandleLoginResult(response, payloadJson, onResult);
	};
	if (username.IsEmpty())
	{
		Fail(onPayload, EApexErrorCode::INVALID_ARGUMENT, TEXT("No username or email entered."));
		return;
	}

	if (RequireClient(onPayload))
	{
		// The platform rejects an empty password field; 1.x sent this placeholder.
		const FString sentPassword = password.IsEmpty() ? FString(TEXT("<empty>")) : password;
		TrackCall(ApexLogin(m_client.Get(), TCHAR_TO_UTF8(*username), TCHAR_TO_UTF8(*sentPassword),
							&UApexSubsystem::HandleResult, this),
				  MoveTemp(onPayload));
	}
}

void UApexSubsystem::LoginWithToken(const FString& userToken, FUserCallback onResult)
{
	FPayloadCallback onPayload = [this, onResult](const FApexResponse& response, const FString& payloadJson)
	{
		HandleLoginResult(response, payloadJson, onResult);
	};
	const FString token = userToken.IsEmpty() ? GetLaunchArgument(kTokenArgument) : userToken;
	if (token.IsEmpty())
	{
		Fail(onPayload, EApexErrorCode::INVALID_ARGUMENT, TEXT("No token to log in with."));
		return;
	}

	if (RequireClient(onPayload))
	{
		TrackCall(ApexLoginWithToken(m_client.Get(), TCHAR_TO_UTF8(*token), &UApexSubsystem::HandleResult, this),
				  MoveTemp(onPayload));
	}
}

void UApexSubsystem::QuickIdGetUsers(const FString& serialNumber, FQuickIdUsersCallback onResult)
{
	FPayloadCallback onPayload = [onResult](const FApexResponse& response, const FString& payloadJson)
	{
		if (onResult)
		{
			onResult(response, response.IsSuccess() ? ApexJson::ParseQuickIdUsers(payloadJson) : FApexQuickIdUsers());
		}
	};
	const FString usedSerialNumber = serialNumber.IsEmpty() ? m_deviceSerialNumber : serialNumber;
	if (usedSerialNumber.IsEmpty())
	{
		Fail(onPayload, EApexErrorCode::INVALID_ARGUMENT, TEXT("No device serial number for Quick ID."));
		return;
	}

	if (RequireClient(onPayload))
	{
		TrackCall(
			ApexQuickIdGetUsers(m_client.Get(), TCHAR_TO_UTF8(*usedSerialNumber), &UApexSubsystem::HandleResult, this),
			MoveTemp(onPayload));
	}
}

void UApexSubsystem::QuickIdLogin(const FString& serialNumber, const FString& username, FUserCallback onResult)
{
	FPayloadCallback onPayload = [this, onResult](const FApexResponse& response, const FString& payloadJson)
	{
		HandleLoginResult(response, payloadJson, onResult);
	};
	const FString usedSerialNumber = serialNumber.IsEmpty() ? m_deviceSerialNumber : serialNumber;
	if (usedSerialNumber.IsEmpty() || username.IsEmpty())
	{
		Fail(onPayload, EApexErrorCode::INVALID_ARGUMENT, TEXT("Quick ID needs a serial number and a username."));
		return;
	}

	if (RequireClient(onPayload))
	{
		TrackCall(ApexQuickIdLogin(m_client.Get(), TCHAR_TO_UTF8(*usedSerialNumber), TCHAR_TO_UTF8(*username),
								   &UApexSubsystem::HandleResult, this),
				  MoveTemp(onPayload));
	}
}

void UApexSubsystem::RequestAuthorizationCode(FCodeCallback onCodeReceived, FUserCallback onResult)
{
	if (!m_authorizationCodeSocket.IsValid())
	{
		m_authorizationCodeSocket = MakeUnique<FApexAuthorizationCodeSocket>();
	}

	TWeakObjectPtr<UApexSubsystem> weakThis(this);
	m_authorizationCodeSocket->RequestCode(
		FApexAuthorizationCodeSocket::MakeWebSocketUrl(GetHeadsetApiUrl()),
		FApexAuthorizationCodeSocket::FCodeReceived::CreateLambda(
			[weakThis, onCodeReceived](const FString& authorizationCode)
	{
		if (onCodeReceived)
		{
			onCodeReceived(authorizationCode);
		}

		if (weakThis.IsValid())
		{
			weakThis->OnAuthorizationCodeReceived.Broadcast(authorizationCode);
		}
	}),
		FApexAuthorizationCodeSocket::FTokenReceived::CreateLambda(
			[weakThis, onResult](const FString& userToken)
	{
		if (weakThis.IsValid())
		{
			weakThis->LoginWithToken(userToken, onResult);
		}
	}),
		FApexAuthorizationCodeSocket::FFailed::CreateLambda([weakThis, onResult](const FString& reason)
	{
		const FApexResponse response =
			FApexResponse::MakeFailure(EApexErrorCode::NETWORK, TEXT("Login by code failed: ") + reason);
		if (onResult)
		{
			onResult(response, FApexUser());
		}

		if (weakThis.IsValid())
		{
			weakThis->OnLoginFailed.Broadcast(response);
		}
	}));
}

void UApexSubsystem::GenerateLoginCode(int32 userId, FLoginCodeCallback onResult)
{
	FPayloadCallback onPayload = [onResult](const FApexResponse& response, const FString& payloadJson)
	{
		if (onResult)
		{
			onResult(response, response.IsSuccess() ? ApexJson::ParseLoginCode(payloadJson) : FApexLoginCode());
		}
	};
	if (RequireClient(onPayload))
	{
		TrackCall(ApexGenerateLoginCode(m_client.Get(), userId, &UApexSubsystem::HandleResult, this),
				  MoveTemp(onPayload));
	}
}

void UApexSubsystem::Logout()
{
	const bool wasLoggedIn = m_isLoggedIn;
	if (m_client.IsValid())
	{
		ApexLogout(m_client.Get());
	}

	m_currentUser = FApexUser();
	m_isLoggedIn = false;
	m_moduleAccess = FApexModuleAccess();
	m_sessionInProgress = false;
	m_sessionId = 0;
	if (wasLoggedIn)
	{
		OnLoggedOutNative.Broadcast();
		OnLoggedOut.Broadcast();
	}
}

void UApexSubsystem::HandleLoginResult(const FApexResponse& response, const FString& payloadJson,
									   FUserCallback onResult)
{
	if (!response.IsSuccess())
	{
		UE_LOG(LogApex, Log, TEXT("Login failed: %s"), *response.Message);
		m_currentUser = FApexUser();
		m_isLoggedIn = false;
		if (onResult)
		{
			onResult(response, FApexUser());
		}

		OnLoginFailed.Broadcast(response);
		return;
	}

	m_currentUser = ApexJson::ParseUser(payloadJson);
	m_isLoggedIn = true;
	m_moduleAccess = FApexModuleAccess();
	OnUserIdentifiedNative.Broadcast(m_currentUser);

	if (!GetDefault<UApexSDKSettings>()->bCheckModuleAccessOnLogin)
	{
		if (onResult)
		{
			onResult(response, m_currentUser);
		}

		OnLoginSucceeded.Broadcast(m_currentUser);
		return;
	}

	// The login reports success once the access check is done.
	CheckModuleAccess(m_moduleId, [this, onResult](const FApexResponse& accessResponse, const FApexModuleAccess&)
	{
		if (!accessResponse.IsSuccess())
		{
			if (onResult)
			{
				onResult(accessResponse, m_currentUser);
			}

			OnLoginFailed.Broadcast(accessResponse);
			return;
		}

		if (onResult)
		{
			onResult(accessResponse, m_currentUser);
		}

		OnLoginSucceeded.Broadcast(m_currentUser);
	});
}

// ---------------------------------------------------------------- Modules

FApexModuleAccess UApexSubsystem::GetModuleAccess() const
{
	return m_moduleAccess;
}

bool UApexSubsystem::HasModuleAccess() const
{
	return m_isLoggedIn && m_moduleAccess.bHasAccess;
}

void UApexSubsystem::CheckModuleAccess(int32 moduleId, FModuleAccessCallback onResult)
{
	const int32 checkedModuleId = moduleId < 0 ? m_moduleId : moduleId;
	FPayloadCallback onPayload =
		[this, checkedModuleId, onResult](const FApexResponse& response, const FString& payloadJson)
	{
		FApexModuleAccess moduleAccess;
		if (response.IsSuccess())
		{
			moduleAccess = ApexJson::ParseModuleAccess(payloadJson);
			if (checkedModuleId == m_moduleId)
			{
				ApplyModuleAccess(moduleAccess);
			}
		}

		if (onResult)
		{
			onResult(response, moduleAccess);
		}
	};
	if (RequireClient(onPayload))
	{
		TrackCall(ApexCheckModuleAccess(m_client.Get(), checkedModuleId, &UApexSubsystem::HandleResult, this),
				  MoveTemp(onPayload));
	}
}

void UApexSubsystem::ApplyModuleAccess(const FApexModuleAccess& moduleAccess)
{
	m_moduleAccess = moduleAccess;
	if (moduleAccess.bHasPassingScore)
	{
		m_currentUser.MinimumPassingScore = moduleAccess.PassingScore;
	}
}

void UApexSubsystem::GetUserModules(int32 userId, FUserModulesCallback onResult)
{
	FPayloadCallback onPayload = [onResult](const FApexResponse& response, const FString& payloadJson)
	{
		if (onResult)
		{
			onResult(response,
					 response.IsSuccess() ? ApexJson::ParseUserModules(payloadJson) : TArray<FApexUserModules>());
		}
	};
	if (RequireClient(onPayload))
	{
		TrackCall(ApexGetUserModules(m_client.Get(), userId, &UApexSubsystem::HandleResult, this), MoveTemp(onPayload));
	}
}

void UApexSubsystem::GetModuleList(const FString& platformName, FModuleListCallback onResult)
{
	FPayloadCallback onPayload = [onResult](const FApexResponse& response, const FString& payloadJson)
	{
		if (onResult)
		{
			onResult(response, response.IsSuccess() ? ApexJson::ParseModuleList(payloadJson) : TArray<FApexModule>());
		}
	};
	if (RequireClient(onPayload))
	{
		TrackCall(ApexGetModuleList(m_client.Get(), TCHAR_TO_UTF8(*platformName), &UApexSubsystem::HandleResult, this),
				  MoveTemp(onPayload));
	}
}

void UApexSubsystem::GetModulePage(const FString& platformName, EApexModuleLifecycle lifecycles,
								   EApexModuleSortOrder sortOrder, int32 pageIndex, int32 pageSize, bool refresh,
								   FModulePageCallback onResult)
{
	FPayloadCallback onPayload = [onResult](const FApexResponse& response, const FString& payloadJson)
	{
		if (onResult)
		{
			onResult(response, response.IsSuccess() ? ApexJson::ParseModulePage(payloadJson) : FApexModulePage());
		}
	};
	if (RequireClient(onPayload))
	{
		TrackCall(ApexGetModulePage(m_client.Get(), TCHAR_TO_UTF8(*platformName), static_cast<uint32_t>(lifecycles),
									static_cast<int32_t>(sortOrder), pageIndex, pageSize, refresh ? 1 : 0,
									&UApexSubsystem::HandleResult, this),
				  MoveTemp(onPayload));
	}
}

// ---------------------------------------------------------------- Sessions

bool UApexSubsystem::IsSessionInProgress() const
{
	return m_sessionInProgress;
}

int64 UApexSubsystem::GetSessionId() const
{
	return m_sessionInProgress ? m_sessionId : 0;
}

FString UApexSubsystem::GetScenarioId() const
{
	return m_scenarioId;
}

void UApexSubsystem::SetScenarioId(const FString& scenarioId)
{
	m_scenarioId = scenarioId;
}

bool UApexSubsystem::GetSendXapiSessionEvents() const
{
	return m_sendXapiSessionEvents;
}

void UApexSubsystem::SetSendXapiSessionEvents(bool sendXapiSessionEvents)
{
	m_sendXapiSessionEvents = sendXapiSessionEvents;
	if (m_client.IsValid())
	{
		ApexSetSessionFormat(m_client.Get(),
							 sendXapiSessionEvents ? APEX_SESSION_FORMAT_XAPI : APEX_SESSION_FORMAT_DATA);
	}
}

bool UApexSubsystem::CanSendSessionCalls(bool needsActiveSession, const FPayloadCallback& onResult) const
{
	if (!m_isLoggedIn)
	{
		Fail(onResult, EApexErrorCode::NOT_LOGGED_IN, TEXT("Session calls need a logged-in user."));
		return false;
	}

	if (GetDefault<UApexSDKSettings>()->bRequireModuleAccessForSessions && !m_moduleAccess.bHasAccess)
	{
		Fail(onResult, EApexErrorCode::INVALID_STATE, TEXT("The user's access to this module is not verified."));
		return false;
	}

	if (needsActiveSession && !m_sessionInProgress)
	{
		Fail(onResult, EApexErrorCode::INVALID_STATE, TEXT("No session in progress."));
		return false;
	}

	return true;
}

TSharedRef<FJsonObject> UApexSubsystem::MakeJoinData(const TSharedPtr<FJsonObject>& sessionData) const
{
	TSharedRef<FJsonObject> joinData = MakeShared<FJsonObject>();
	joinData->SetStringField(TEXT("scenarioId"), m_scenarioId);
	joinData->SetStringField(TEXT("moduleName"), m_moduleName);
	joinData->SetStringField(TEXT("moduleVersion"), m_moduleVersion);
	joinData->SetStringField(TEXT("platform"), m_platformName);
	joinData->SetStringField(TEXT("deviceId"), m_deviceId);
	joinData->SetStringField(TEXT("deviceModel"), m_deviceModel);
	SetStringOrNull(joinData, TEXT("deviceSerial"), m_deviceSerialNumber);
	joinData->SetStringField(TEXT("sdkVersion"), TEXT("unreal-") + GetSdkVersion());
	joinData->SetField(TEXT("sdkSourceHash"), MakeShared<FJsonValueNull>());
	joinData->SetStringField(TEXT("coreVersion"), GetCoreVersion());
	joinData->SetBoolField(TEXT("moduleAccessChecked"),
						   GetDefault<UApexSDKSettings>()->bRequireModuleAccessForSessions);
	SetObjectOrNull(joinData, TEXT("data"), sessionData);
	return joinData;
}

void UApexSubsystem::JoinSession(const FString& scenarioId, const FString& sessionDataJson, FSessionCallback onResult)
{
	TSharedPtr<FJsonObject> sessionData;
	if (!TryParseOptionalObject(sessionDataJson, sessionData))
	{
		Fail([onResult](const FApexResponse& response, const FString&)
		{
			if (onResult)
			{
				onResult(response, 0);
			}
		}, EApexErrorCode::INVALID_ARGUMENT, TEXT("The session data is not a JSON object."));
		return;
	}

	if (!scenarioId.IsEmpty())
	{
		m_scenarioId = scenarioId;
	}

	JoinSessionWithRawData(ToUtf8Bytes(ApexJson::Write(MakeJoinData(sessionData))), MoveTemp(onResult));
}

void UApexSubsystem::JoinSessionWithRawData(const TArray<uint8>& moduleData, FSessionCallback onResult)
{
	FPayloadCallback onPayload = [this, onResult](const FApexResponse& response, const FString& payloadJson)
	{
		int64 sessionId = 0;
		if (response.IsSuccess())
		{
			sessionId = ApexJson::ParseSessionId(payloadJson);
			m_sessionInProgress = true;
			m_sessionId = sessionId;
			UE_LOG(LogApex, Log, TEXT("Session %lld joined."), static_cast<long long>(sessionId));
		}

		if (onResult)
		{
			onResult(response, sessionId);
		}

		if (response.IsSuccess())
		{
			OnSessionJoinedNative.Broadcast(sessionId);
			OnSessionJoined.Broadcast(sessionId);
		}
	};
	if (!RequireClient(onPayload) || !CanSendSessionCalls(false, onPayload))
	{
		return;
	}

	if (m_sessionInProgress)
	{
		UE_LOG(LogApex, Warning, TEXT("A session is already in progress; joining a new one."));
	}

	TrackCall(ApexSessionJoin(m_client.Get(), moduleData.GetData(), static_cast<size_t>(moduleData.Num()),
							  &UApexSubsystem::HandleResult, this),
			  MoveTemp(onPayload));
}

void UApexSubsystem::SendSessionEvent(const FString& action, const FString& targetObject, const FString& eventDataJson,
									  FResultCallback onResult)
{
	const FPayloadCallback onFailure = [onResult](const FApexResponse& response, const FString&)
	{
		if (onResult)
		{
			onResult(response);
		}
	};
	TSharedPtr<FJsonObject> eventData;
	if (action.IsEmpty())
	{
		Fail(onFailure, EApexErrorCode::INVALID_ARGUMENT, TEXT("A session event needs an action."));
		return;
	}

	if (!TryParseOptionalObject(eventDataJson, eventData))
	{
		Fail(onFailure, EApexErrorCode::INVALID_ARGUMENT, TEXT("The event data is not a JSON object."));
		return;
	}

	TSharedRef<FJsonObject> eventBody = MakeShared<FJsonObject>();
	eventBody->SetStringField(TEXT("action"), action);
	SetStringOrNull(eventBody, TEXT("target"), targetObject);
	eventBody->SetStringField(TEXT("scenarioId"), m_scenarioId);
	SetObjectOrNull(eventBody, TEXT("data"), eventData);
	if (CanSendSessionCalls(true, nullptr))
	{
		OnSessionEventNative.Broadcast(action, targetObject, eventDataJson);
	}

	SendSessionEventWithRawData(ToUtf8Bytes(ApexJson::Write(eventBody)), MoveTemp(onResult));
}

void UApexSubsystem::SendSessionStatement(const FString& statementJson, FResultCallback onResult)
{
	const FPayloadCallback onFailure = [onResult](const FApexResponse& response, const FString&)
	{
		if (onResult)
		{
			onResult(response);
		}
	};
	if (!m_sendXapiSessionEvents)
	{
		Fail(onFailure, EApexErrorCode::INVALID_STATE, TEXT("Statements need xAPI session events."));
		return;
	}

	const TSharedPtr<FJsonObject> statement = ApexJson::ParseObject(statementJson);
	if (!statement.IsValid() || !statement->HasField(TEXT("verb")) || !statement->HasField(TEXT("object")))
	{
		Fail(onFailure, EApexErrorCode::INVALID_ARGUMENT, TEXT("The statement needs a verb and an object."));
		return;
	}

	SendSessionEventWithRawData(ToUtf8Bytes(statementJson), MoveTemp(onResult));
}

void UApexSubsystem::SendSessionEventWithRawData(const TArray<uint8>& moduleData, FResultCallback onResult)
{
	FPayloadCallback onPayload = [onResult](const FApexResponse& response, const FString&)
	{
		if (onResult)
		{
			onResult(response);
		}
	};
	if (RequireClient(onPayload) && CanSendSessionCalls(true, onPayload))
	{
		TrackCall(ApexSessionEvent(m_client.Get(), moduleData.GetData(), static_cast<size_t>(moduleData.Num()),
								   &UApexSubsystem::HandleResult, this),
				  MoveTemp(onPayload));
	}
}

void UApexSubsystem::CompleteSession(const FApexSessionResult& sessionResult, const FString& resultDataJson,
									 FResultCallback onResult)
{
	TSharedPtr<FJsonObject> resultData;
	if (!TryParseOptionalObject(resultDataJson, resultData))
	{
		Fail([onResult](const FApexResponse& response, const FString&)
		{
			if (onResult)
			{
				onResult(response);
			}
		}, EApexErrorCode::INVALID_ARGUMENT, TEXT("The result data is not a JSON object."));
		return;
	}

	TSharedRef<FJsonObject> completeBody = MakeShared<FJsonObject>();
	completeBody->SetStringField(TEXT("scenarioId"), m_scenarioId);
	completeBody->SetNumberField(TEXT("score"), sessionResult.Score);
	completeBody->SetNumberField(TEXT("scaledScore"), sessionResult.GetEffectiveScaledScore());
	completeBody->SetNumberField(TEXT("minimumScore"), sessionResult.MinimumScore);
	completeBody->SetNumberField(TEXT("maximumScore"), sessionResult.MaximumScore);
	completeBody->SetNumberField(TEXT("durationSeconds"), sessionResult.DurationSeconds);
	completeBody->SetBoolField(TEXT("complete"), sessionResult.bComplete);
	completeBody->SetBoolField(TEXT("success"), sessionResult.bSuccess);
	SetObjectOrNull(completeBody, TEXT("data"), resultData);
	if (CanSendSessionCalls(true, nullptr))
	{
		OnSessionCompletedNative.Broadcast(sessionResult);
	}

	CompleteSessionWithRawData(ToUtf8Bytes(ApexJson::Write(completeBody)), MoveTemp(onResult));
}

void UApexSubsystem::CompleteSessionWithRawData(const TArray<uint8>& moduleData, FResultCallback onResult)
{
	FPayloadCallback onPayload = [this, onResult](const FApexResponse& response, const FString&)
	{
		if (response.IsSuccess())
		{
			m_sessionInProgress = false;
			m_sessionId = 0;
		}

		// On failure the session stays active so the module can retry.
		if (onResult)
		{
			onResult(response);
		}

		if (response.IsSuccess())
		{
			OnSessionCompleted.Broadcast();
		}
	};
	if (RequireClient(onPayload) && CanSendSessionCalls(true, onPayload))
	{
		TrackCall(ApexSessionComplete(m_client.Get(), moduleData.GetData(), static_cast<size_t>(moduleData.Num()),
									  &UApexSubsystem::HandleResult, this),
				  MoveTemp(onPayload));
	}
}

// ---------------------------------------------------------------- Launch arguments and the launcher

FString UApexSubsystem::GetLaunchArgument(const FString& key) const
{
	if (!m_client.IsValid())
	{
		const FString* argumentValue = m_launchArguments.Find(key);
		return argumentValue != nullptr ? *argumentValue : FString();
	}

	const FTCHARToUTF8 utf8Key(*key);
	ApexClient* client = m_client.Get();
	return ReadCoreString([client, &utf8Key](char* buffer, size_t bufferSize)
	{
		return ApexLaunchGet(client, utf8Key.Get(), buffer, bufferSize);
	});
}

void UApexSubsystem::SetLaunchArgument(const FString& key, const FString& value)
{
	m_launchArguments.Add(key, value);
	if (m_client.IsValid())
	{
		ApexLaunchSet(m_client.Get(), TCHAR_TO_UTF8(*key), TCHAR_TO_UTF8(*value));
	}
}

void UApexSubsystem::SetExitTarget(const FString& exitTarget)
{
	SetLaunchArgument(kReturnTargetArgument, exitTarget);
	SetLaunchArgument(kTargetTypeArgument, exitTarget.Contains(TEXT("://")) ? TEXT("url") : TEXT("app"));
}

void UApexSubsystem::SetOptionalData(const FString& optionalData)
{
	SetLaunchArgument(kOptionalArgument, optionalData);
}

void UApexSubsystem::ExitToLauncher(const FString& nextTarget)
{
	FString exitTargetJson = TEXT("{\"action\":\"none\"}");
	if (m_client.IsValid())
	{
		const FTCHARToUTF8 utf8NextTarget(*nextTarget);
		ApexClient* client = m_client.Get();
		exitTargetJson = ReadCoreString([client, &utf8NextTarget, &nextTarget](char* buffer, size_t bufferSize)
		{
			return ApexLaunchExitTarget(client, nextTarget.IsEmpty() ? nullptr : utf8NextTarget.Get(), buffer,
										bufferSize);
		});
	}

	const TSharedPtr<FJsonObject> exitTarget = ApexJson::ParseObject(exitTargetJson);
	const FString exitAction = ApexJson::GetString(exitTarget, TEXT("action"));
	UE_LOG(LogApex, Log, TEXT("Exiting to the launcher: %s."), *exitAction);

	UApexLauncherSubsystem* launcher = UApexLauncherSubsystem::Get();
	bool launched = false;
	if (launcher != nullptr && exitAction == TEXT("open_url"))
	{
		launched = launcher->LaunchUrl(ApexJson::GetString(exitTarget, TEXT("url")), true);
	}
	else if (launcher != nullptr && exitAction == TEXT("open_app"))
	{
		TArray<FString> extraKeys;
		TArray<FString> extraValues;
		const TSharedPtr<FJsonObject> extras = ApexJson::GetObject(exitTarget, TEXT("extras"));
		if (extras.IsValid())
		{
			for (const TPair<FString, TSharedPtr<FJsonValue>>& extra : extras->Values)
			{
				extraKeys.Add(extra.Key);
				extraValues.Add(extra.Value.IsValid() ? extra.Value->AsString() : FString());
			}
		}

		launched =
			launcher->LaunchApplication(ApexJson::GetString(exitTarget, TEXT("package")), extraKeys, extraValues, true);
	}

	if (!launched)
	{
		if (launcher != nullptr)
		{
			launcher->QuitApplication();
		}
		else
		{
			FPlatformMisc::RequestExit(false);
		}
	}
}

bool UApexSubsystem::LaunchModule(const FString& targetApplication, const FString& returnTarget)
{
	UApexLauncherSubsystem* launcher = UApexLauncherSubsystem::Get();
	if (launcher == nullptr || targetApplication.IsEmpty())
	{
		return false;
	}

	const FString userToken = m_isLoggedIn ? m_currentUser.Token : GetLaunchArgument(kTokenArgument);
	TArray<FString> extraKeys = {kTokenArgument};
	TArray<FString> extraValues = {userToken};
	const FString optionalData = GetLaunchArgument(kOptionalArgument);
	if (!optionalData.IsEmpty())
	{
		extraKeys.Add(kOptionalArgument);
		extraValues.Add(optionalData);
	}

	if (!returnTarget.IsEmpty())
	{
		extraKeys.Add(kReturnTargetArgument);
		extraValues.Add(returnTarget);
		extraKeys.Add(kTargetTypeArgument);
		extraValues.Add(returnTarget.Contains(TEXT("://")) ? TEXT("url") : TEXT("app"));
	}

	if (!targetApplication.Contains(TEXT("://")))
	{
		return launcher->LaunchApplication(targetApplication, extraKeys, extraValues, true);
	}

	FString launchUrl = targetApplication;
	for (int32 extraIndex = 0; extraIndex < extraKeys.Num(); ++extraIndex)
	{
		launchUrl += (extraIndex == 0 && !launchUrl.Contains(TEXT("?"))) ? TEXT("?") : TEXT("&");
		launchUrl += extraKeys[extraIndex] + TEXT("=") + FGenericPlatformHttp::UrlEncode(extraValues[extraIndex]);
	}

	return launcher->LaunchUrl(launchUrl, true);
}

// ---------------------------------------------------------------- Telemetry

ApexPose UApexSubsystem::MakeApexPose(const FTransform& worldTransform)
{
	// Unreal: centimeters, X forward, Y right, Z up. Core (Unity's axes): meters, X right, Y up, Z forward.
	const FVector location = worldTransform.GetLocation();
	const FRotator rotation = worldTransform.Rotator();
	ApexPose pose;
	pose.Position[0] = location.Y / 100.0f;
	pose.Position[1] = location.Z / 100.0f;
	pose.Position[2] = location.X / 100.0f;
	// Unity's X turns forward down where Unreal's pitch turns it up; Unity's Z (roll) turns counterclockwise.
	pose.Rotation[0] = FRotator::ClampAxis(-rotation.Pitch);
	pose.Rotation[1] = FRotator::ClampAxis(rotation.Yaw);
	pose.Rotation[2] = FRotator::ClampAxis(-rotation.Roll);
	return pose;
}

bool UApexSubsystem::IsTelemetryRecording() const
{
	return m_client.IsValid() && m_telemetrySink != EApexTelemetrySinkType::NONE;
}

void UApexSubsystem::TelemetryObjectRegistered(const FString& trackedId, const FString& objectName,
											   const FString& meshName, const FTransform& worldTransform)
{
	if (IsTelemetryRecording())
	{
		const ApexPose pose = MakeApexPose(worldTransform);
		ApexTelemetryObjectRegistered(m_client.Get(), TCHAR_TO_UTF8(*trackedId), TCHAR_TO_UTF8(*objectName),
									  TCHAR_TO_UTF8(*meshName), &pose);
	}
}

void UApexSubsystem::TelemetryObjectUnregistered(const FString& trackedId, const FTransform& worldTransform)
{
	if (IsTelemetryRecording())
	{
		const ApexPose pose = MakeApexPose(worldTransform);
		ApexTelemetryObjectUnregistered(m_client.Get(), TCHAR_TO_UTF8(*trackedId), &pose);
	}
}

void UApexSubsystem::TelemetryEngagementBegin(const FString& trackedId, const FString& engagementName)
{
	if (IsTelemetryRecording())
	{
		ApexTelemetryEngagementBegin(m_client.Get(), TCHAR_TO_UTF8(*trackedId), TCHAR_TO_UTF8(*engagementName));
	}
}

void UApexSubsystem::TelemetryEngagementEnd(const FString& trackedId, const FString& engagementName)
{
	if (IsTelemetryRecording())
	{
		ApexTelemetryEngagementEnd(m_client.Get(), TCHAR_TO_UTF8(*trackedId), TCHAR_TO_UTF8(*engagementName));
	}
}

void UApexSubsystem::TelemetryInteraction(const FString& trackedId, const FString& actionName)
{
	if (IsTelemetryRecording())
	{
		ApexTelemetryInteraction(m_client.Get(), TCHAR_TO_UTF8(*trackedId), TCHAR_TO_UTF8(*actionName));
	}
}

void UApexSubsystem::TelemetryStepBegin(const FString& stepName)
{
	if (IsTelemetryRecording())
	{
		ApexTelemetryStepBegin(m_client.Get(), TCHAR_TO_UTF8(*stepName));
	}
}

void UApexSubsystem::TelemetryStepEnd(const FString& stepName, bool succeeded, double score)
{
	if (IsTelemetryRecording())
	{
		ApexTelemetryStepEnd(m_client.Get(), TCHAR_TO_UTF8(*stepName), succeeded ? 1 : 0, score);
	}
}

bool UApexSubsystem::IsTelemetryPoseDue()
{
	return IsTelemetryRecording() && ApexTelemetryPoseDue(m_client.Get()) != 0;
}

void UApexSubsystem::TelemetryPose(const FString& trackedId, const FTransform& worldTransform)
{
	if (IsTelemetryRecording())
	{
		const ApexPose pose = MakeApexPose(worldTransform);
		ApexTelemetryPose(m_client.Get(), trackedId.IsEmpty() ? nullptr : TCHAR_TO_UTF8(*trackedId), &pose);
	}
}

void UApexSubsystem::TelemetryFlush()
{
	if (IsTelemetryRecording())
	{
		ApexTelemetryFlush(m_client.Get());
	}
}
