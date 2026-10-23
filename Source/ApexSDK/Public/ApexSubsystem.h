// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ApexTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "apex/Apex.h"
#include "ApexSubsystem.generated.h"

class FApexAuthorizationCodeSocket;
class FJsonObject;
class FApexUnrealHttp;
class UApexSDKSettings;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FApexUserDelegate, const FApexUser&, User);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FApexResponseDelegate, const FApexResponse&, Response);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FApexSimpleDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FApexSessionDelegate, int64, SessionId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FApexCodeDelegate, const FString&, AuthorizationCode);

DECLARE_MULTICAST_DELEGATE_OneParam(FApexUserNativeDelegate, const FApexUser&);
DECLARE_MULTICAST_DELEGATE(FApexSimpleNativeDelegate);
DECLARE_MULTICAST_DELEGATE_OneParam(FApexSessionNativeDelegate, int64);
/** Action, target object and the module's data (JSON, may be empty). */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FApexSessionEventNativeDelegate, const FString&, const FString&, const FString&);
DECLARE_MULTICAST_DELEGATE_OneParam(FApexSessionResultNativeDelegate, const FApexSessionResult&);

/**
 * The Apex platform for one game instance: login, module access and lists, sessions, telemetry and
 * the launcher hand-off. Every call runs in apex-sdk-core; its result arrives on a later frame, on
 * the game thread, through the callback given to the call (C++) or the async node (Blueprints).
 *
 * The subsystem starts with the game instance, from Project Settings > Plugins > Apex SDK. It reads
 * the launch arguments (the launcher's "pixotoken" and where to return), polls the core every frame
 * (also while paused), and tells analytics providers what happened through the native delegates.
 */
UCLASS(DisplayName = "Apex")
class APEXSDK_API UApexSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	using FResultCallback = TFunction<void(const FApexResponse& response)>;
	using FPayloadCallback = TFunction<void(const FApexResponse& response, const FString& payloadJson)>;
	using FUserCallback = TFunction<void(const FApexResponse& response, const FApexUser& user)>;
	using FModuleAccessCallback = TFunction<void(const FApexResponse& response, const FApexModuleAccess& access)>;
	using FQuickIdUsersCallback = TFunction<void(const FApexResponse& response, const FApexQuickIdUsers& users)>;
	using FLoginCodeCallback = TFunction<void(const FApexResponse& response, const FApexLoginCode& loginCode)>;
	using FUserModulesCallback =
		TFunction<void(const FApexResponse& response, const TArray<FApexUserModules>& usersModules)>;
	using FModuleListCallback = TFunction<void(const FApexResponse& response, const TArray<FApexModule>& modules)>;
	using FModulePageCallback = TFunction<void(const FApexResponse& response, const FApexModulePage& modulePage)>;
	using FSessionCallback = TFunction<void(const FApexResponse& response, int64 sessionId)>;
	using FCodeCallback = TFunction<void(const FString& authorizationCode)>;

	UApexSubsystem();
	virtual ~UApexSubsystem();

	/** The subsystem of the world's game instance, or nullptr. */
	static UApexSubsystem* Get(const UObject* worldContextObject);

	virtual void Initialize(FSubsystemCollectionBase& collection) override;
	virtual void Deinitialize() override;

	virtual void Tick(float deltaSeconds) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;
	virtual bool IsTickableWhenPaused() const override;
	virtual bool IsTickableInEditor() const override;

	// ------------------------------------------------------------ Core and environment

	/** True when the core client is running. */
	UFUNCTION(BlueprintPure, Category = "Apex")
	bool IsCoreAvailable() const;

	/** The apex-sdk-core version, e.g. "0.2.0". */
	UFUNCTION(BlueprintPure, Category = "Apex")
	static FString GetCoreVersion();

	/** This plugin's version, e.g. "2.0.0". */
	UFUNCTION(BlueprintPure, Category = "Apex")
	static FString GetSdkVersion();

	UFUNCTION(BlueprintPure, Category = "Apex")
	EApexServerEnvironment GetEnvironment() const;

	/** Switches environment. The new client starts logged out; launch arguments are kept. */
	UFUNCTION(BlueprintCallable, Category = "Apex")
	void ChangeEnvironment(EApexServerEnvironment newEnvironment);

	/** The Headset API URL in use (login, access, sessions). */
	UFUNCTION(BlueprintPure, Category = "Apex")
	FString GetHeadsetApiUrl() const;

	UFUNCTION(BlueprintPure, Category = "Apex")
	int32 GetModuleId() const;

	UFUNCTION(BlueprintPure, Category = "Apex")
	FString GetModuleName() const;

	UFUNCTION(BlueprintPure, Category = "Apex")
	FString GetModuleVersion() const;

	UFUNCTION(BlueprintPure, Category = "Apex|Device")
	FString GetDeviceId() const;

	UFUNCTION(BlueprintPure, Category = "Apex|Device")
	FString GetDeviceModel() const;

	/** The XR system's name when one is running, else the platform's ("Windows", "Android"). */
	UFUNCTION(BlueprintPure, Category = "Apex|Device")
	FString GetPlatformName() const;

	/** The serial number Quick ID and module access use. On Android it is read from ManageXR. */
	UFUNCTION(BlueprintPure, Category = "Apex|Device")
	FString GetDeviceSerialNumber() const;

	UFUNCTION(BlueprintCallable, Category = "Apex|Device")
	void SetDeviceSerialNumber(const FString& serialNumber);

	// ------------------------------------------------------------ Login

	UFUNCTION(BlueprintPure, Category = "Apex|Login")
	bool IsLoggedIn() const;

	UFUNCTION(BlueprintPure, Category = "Apex|Login")
	FApexUser GetCurrentUser() const;

	/** True when the launcher passed a login token ("pixotoken"). */
	UFUNCTION(BlueprintPure, Category = "Apex|Login")
	bool HasLaunchLoginToken() const;

	void Ping(FResultCallback onResult = nullptr);
	void Login(const FString& username, const FString& password, FUserCallback onResult = nullptr);
	/** An empty token uses the launcher's ("pixotoken"). */
	void LoginWithToken(const FString& userToken, FUserCallback onResult = nullptr);
	/** An empty serial number uses this device's. */
	void QuickIdGetUsers(const FString& serialNumber, FQuickIdUsersCallback onResult = nullptr);
	void QuickIdLogin(const FString& serialNumber, const FString& username, FUserCallback onResult = nullptr);

	/**
	 * Login by code: onCodeReceived gets the code to show; once the user enters it on another
	 * device, the SDK logs in with the token the platform hands over and calls onResult.
	 */
	void RequestAuthorizationCode(FCodeCallback onCodeReceived, FUserCallback onResult = nullptr);

	/** A one-time login code; userId < 0 is the current user. */
	void GenerateLoginCode(int32 userId, FLoginCodeCallback onResult = nullptr);

	/** Forgets the user and ends any session locally. */
	UFUNCTION(BlueprintCallable, Category = "Apex|Login")
	void Logout();

	// ------------------------------------------------------------ Modules

	/** The last access check for this module, made after login by default. */
	UFUNCTION(BlueprintPure, Category = "Apex|Modules")
	FApexModuleAccess GetModuleAccess() const;

	UFUNCTION(BlueprintPure, Category = "Apex|Modules")
	bool HasModuleAccess() const;

	/** moduleId < 0 is this module. */
	void CheckModuleAccess(int32 moduleId, FModuleAccessCallback onResult = nullptr);
	/** userId < 0 is the current user. */
	void GetUserModules(int32 userId, FUserModulesCallback onResult = nullptr);
	/** The user's modules, with versions built for platformName, Archived left out. */
	void GetModuleList(const FString& platformName, FModuleListCallback onResult = nullptr);
	/**
	 * One page of the user's modules: versions built for platformName (empty: any) whose lifecycle is
	 * in lifecycles (none set: RELEASED). The first call fetches the list; later pages reuse it until
	 * logout, the next login, GetModuleList or refresh. A page served that way has HttpStatus 0.
	 */
	void GetModulePage(const FString& platformName, EApexModuleLifecycle lifecycles, EApexModuleSortOrder sortOrder,
					   int32 pageIndex, int32 pageSize, bool refresh, FModulePageCallback onResult = nullptr);

	// ------------------------------------------------------------ Sessions

	UFUNCTION(BlueprintPure, Category = "Apex|Sessions")
	bool IsSessionInProgress() const;

	/** The platform's ID for the active session, or 0. */
	UFUNCTION(BlueprintPure, Category = "Apex|Sessions")
	int64 GetSessionId() const;

	UFUNCTION(BlueprintPure, Category = "Apex|Sessions")
	FString GetScenarioId() const;

	UFUNCTION(BlueprintCallable, Category = "Apex|Sessions")
	void SetScenarioId(const FString& scenarioId);

	UFUNCTION(BlueprintPure, Category = "Apex|Sessions")
	bool GetSendXapiSessionEvents() const;

	/** Applies from the next join; an active session keeps its format. */
	UFUNCTION(BlueprintCallable, Category = "Apex|Sessions")
	void SetSendXapiSessionEvents(bool sendXapiSessionEvents);

	/**
	 * Starts a session. scenarioId (when not empty) becomes the session's scenario; it, the module and
	 * device details and sessionDataJson (a JSON object, or empty) go to the platform as join data.
	 */
	void JoinSession(const FString& scenarioId, const FString& sessionDataJson, FSessionCallback onResult = nullptr);
	/** An action during the session, optionally on a target object, with the module's JSON data. */
	void SendSessionEvent(const FString& action, const FString& targetObject, const FString& eventDataJson,
						  FResultCallback onResult = nullptr);
	/** An xAPI statement (JSON with a verb and an object); needs xAPI session events. */
	void SendSessionStatement(const FString& statementJson, FResultCallback onResult = nullptr);
	/** Ends the session; on failure it stays active so the module can retry. */
	void CompleteSession(const FApexSessionResult& sessionResult, const FString& resultDataJson,
						 FResultCallback onResult = nullptr);

	/** The session calls with the module's own bytes, sent as given (JSON objects with xAPI). */
	void JoinSessionWithRawData(const TArray<uint8>& moduleData, FSessionCallback onResult = nullptr);
	void SendSessionEventWithRawData(const TArray<uint8>& moduleData, FResultCallback onResult = nullptr);
	void CompleteSessionWithRawData(const TArray<uint8>& moduleData, FResultCallback onResult = nullptr);

	// ------------------------------------------------------------ Launch arguments and the launcher

	/** A launch argument ("pixotoken", "optional", "returntarget", "targettype", ...), or "". */
	UFUNCTION(BlueprintPure, Category = "Apex|Launcher")
	FString GetLaunchArgument(const FString& key) const;

	UFUNCTION(BlueprintCallable, Category = "Apex|Launcher")
	void SetLaunchArgument(const FString& key, const FString& value);

	/** Where the module returns when it exits: an app package or a URL. */
	UFUNCTION(BlueprintCallable, Category = "Apex|Launcher")
	void SetExitTarget(const FString& exitTarget);

	/** The launcher's "optional" data, passed back when the module exits. */
	UFUNCTION(BlueprintCallable, Category = "Apex|Launcher")
	void SetOptionalData(const FString& optionalData);

	/**
	 * Hands control back to the launcher (or the app it named), passing the login on, then quits.
	 * nextTarget (optional) is passed to the launcher as "returntarget".
	 */
	UFUNCTION(BlueprintCallable, Category = "Apex|Launcher")
	void ExitToLauncher(const FString& nextTarget = TEXT(""));

	/**
	 * Launches another module (an Android package or a URL), passing the current login token and
	 * optional data; returnTarget is where that module returns when it exits.
	 */
	UFUNCTION(BlueprintCallable, Category = "Apex|Launcher")
	bool LaunchModule(const FString& targetApplication, const FString& returnTarget = TEXT(""));

	// ------------------------------------------------------------ Telemetry (used by ApexAnalytics)

	void TelemetryObjectRegistered(const FString& trackedId, const FString& objectName, const FString& meshName,
								   const FTransform& worldTransform);
	void TelemetryObjectUnregistered(const FString& trackedId, const FTransform& worldTransform);
	void TelemetryEngagementBegin(const FString& trackedId, const FString& engagementName);
	void TelemetryEngagementEnd(const FString& trackedId, const FString& engagementName);
	void TelemetryInteraction(const FString& trackedId, const FString& actionName);
	void TelemetryStepBegin(const FString& stepName);
	void TelemetryStepEnd(const FString& stepName, bool succeeded, double score);
	/** True at most PoseSamplesPerSecond times a second: report the head and tracked poses then. */
	bool IsTelemetryPoseDue();
	/** An empty trackedId is the head. */
	void TelemetryPose(const FString& trackedId, const FTransform& worldTransform);
	void TelemetryFlush();
	/** True when the core records telemetry (a sink is set and the PixoVR provider is on). */
	bool IsTelemetryRecording() const;

	/** Unreal world transform (cm, X forward, Z up) to the core's pose (m, Unity's axes, Euler degrees). */
	static ApexPose MakeApexPose(const FTransform& worldTransform);

	// ------------------------------------------------------------ Events

	UPROPERTY(BlueprintAssignable, Category = "Apex|Events")
	FApexUserDelegate OnLoginSucceeded;

	UPROPERTY(BlueprintAssignable, Category = "Apex|Events")
	FApexResponseDelegate OnLoginFailed;

	UPROPERTY(BlueprintAssignable, Category = "Apex|Events")
	FApexSimpleDelegate OnLoggedOut;

	UPROPERTY(BlueprintAssignable, Category = "Apex|Events")
	FApexSessionDelegate OnSessionJoined;

	UPROPERTY(BlueprintAssignable, Category = "Apex|Events")
	FApexSimpleDelegate OnSessionCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Apex|Events")
	FApexCodeDelegate OnAuthorizationCodeReceived;

	/** For analytics providers; same moments as the Blueprint events, with the session details. */
	FApexUserNativeDelegate OnUserIdentifiedNative;
	FApexSimpleNativeDelegate OnLoggedOutNative;
	FApexSessionNativeDelegate OnSessionJoinedNative;
	FApexSessionEventNativeDelegate OnSessionEventNative;
	FApexSessionResultNativeDelegate OnSessionCompletedNative;

private:
	struct FApexClientDeleter
	{
		void operator()(ApexClient* client) const;
	};

	void CreateClient();
	void DestroyClient();
	void ReadDeviceInformation();
	void ReadPixoConfigFile();
	void ReadLaunchArguments();
	void ApplyLaunchArgument(const FString& key, const FString& value);
	void PollDeviceSerialNumber(double nowSeconds);

	/** Remembers onResult for the call and returns true, or delivers a failure for a call that has none. */
	bool TrackCall(ApexCallId callId, FPayloadCallback onResult);
	bool RequireClient(const FPayloadCallback& onResult);
	bool CanSendSessionCalls(bool needsActiveSession, const FPayloadCallback& onResult) const;
	void HandleLoginResult(const FApexResponse& response, const FString& payloadJson, FUserCallback onResult);
	void ApplyModuleAccess(const FApexModuleAccess& moduleAccess);
	TSharedRef<FJsonObject> MakeJoinData(const TSharedPtr<FJsonObject>& sessionData) const;

	static void APEX_CALL HandleResult(void* userData, const ApexResult* result);
	static void APEX_CALL HandleLog(void* userData, int32_t logLevel, const char* message);
	static void APEX_CALL HandleTelemetryPacket(void* userData, const char* packetJson);

	static FApexResponse MakeResponse(const ApexResult& result);
	static void Fail(const FPayloadCallback& onResult, EApexErrorCode error, const FString& message);

	TUniquePtr<ApexClient, FApexClientDeleter> m_client;
	TUniquePtr<FApexUnrealHttp> m_unrealHttp;
	TUniquePtr<FApexAuthorizationCodeSocket> m_authorizationCodeSocket;
	TMap<ApexCallId, FPayloadCallback> m_pendingCalls;

	EApexServerEnvironment m_environment;
	int32 m_moduleId;
	FString m_moduleName;
	FString m_moduleVersion;
	FString m_scenarioId;
	bool m_sendXapiSessionEvents;
	EApexTelemetrySinkType m_telemetrySink;
	FString m_telemetryDirectory;

	FString m_deviceId;
	FString m_deviceModel;
	FString m_platformName;
	FString m_deviceSerialNumber;
	double m_nextSerialNumberCheckSeconds;

	/** Launch arguments, kept across clients. */
	TMap<FString, FString> m_launchArguments;

	FApexUser m_currentUser;
	bool m_isLoggedIn;
	FApexModuleAccess m_moduleAccess;
	bool m_sessionInProgress;
	int64 m_sessionId;
};
