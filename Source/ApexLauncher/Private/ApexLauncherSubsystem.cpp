// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#include "ApexLauncherSubsystem.h"

#include "ApexLauncherSettings.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformProcess.h"

#if PLATFORM_ANDROID
#include "Android/AndroidApplication.h"
#include "Android/AndroidJNI.h"
#include "Android/AndroidJavaEnv.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogApexLauncher, Log, All);

#if PLATFORM_ANDROID
namespace ApexLauncherJava
{
/** The GameActivity method added by ApexLauncherAndroid_UPL.xml, or nullptr (logged) when it is missing. */
static jmethodID FindActivityMethod(JNIEnv* javaEnvironment, const char* methodName, const char* signature)
{
	jmethodID methodId =
		FJavaWrapper::FindMethod(javaEnvironment, FJavaWrapper::GameActivityClassID, methodName, signature, true);
	if (methodId == nullptr)
	{
		UE_LOG(LogApexLauncher, Error, TEXT("GameActivity has no %s; is the ApexLauncher UPL applied?"),
			   UTF8_TO_TCHAR(methodName));
	}

	return methodId;
}

static bool CallBoolean(const char* methodName, const char* signature, ...)
{
	JNIEnv* javaEnvironment = FAndroidApplication::GetJavaEnv();
	jmethodID methodId =
		javaEnvironment != nullptr ? FindActivityMethod(javaEnvironment, methodName, signature) : nullptr;
	if (methodId == nullptr)
	{
		return false;
	}

	va_list javaArguments;
	va_start(javaArguments, signature);
	jboolean callResult = javaEnvironment->CallBooleanMethodV(FJavaWrapper::GameActivityThis, methodId, javaArguments);
	va_end(javaArguments);
	if (javaEnvironment->ExceptionCheck())
	{
		javaEnvironment->ExceptionDescribe();
		javaEnvironment->ExceptionClear();
		return false;
	}

	return callResult == JNI_TRUE;
}

static FString CallString(const char* methodName, const char* signature, ...)
{
	JNIEnv* javaEnvironment = FAndroidApplication::GetJavaEnv();
	jmethodID methodId =
		javaEnvironment != nullptr ? FindActivityMethod(javaEnvironment, methodName, signature) : nullptr;
	if (methodId == nullptr)
	{
		return FString();
	}

	va_list javaArguments;
	va_start(javaArguments, signature);
	jobject callResult = javaEnvironment->CallObjectMethodV(FJavaWrapper::GameActivityThis, methodId, javaArguments);
	va_end(javaArguments);
	if (javaEnvironment->ExceptionCheck())
	{
		javaEnvironment->ExceptionDescribe();
		javaEnvironment->ExceptionClear();
		return FString();
	}

	return FJavaHelper::FStringFromLocalRef(javaEnvironment, static_cast<jstring>(callResult));
}

static void CallVoid(const char* methodName)
{
	JNIEnv* javaEnvironment = FAndroidApplication::GetJavaEnv();
	jmethodID methodId = javaEnvironment != nullptr ? FindActivityMethod(javaEnvironment, methodName, "()V") : nullptr;
	if (methodId != nullptr)
	{
		FJavaWrapper::CallVoidMethod(javaEnvironment, FJavaWrapper::GameActivityThis, methodId);
	}
}

static jobjectArray MakeStringArray(JNIEnv* javaEnvironment, const TArray<FString>& strings)
{
	jobjectArray javaArray = javaEnvironment->NewObjectArray(strings.Num(), FJavaWrapper::JavaStringClass, nullptr);
	for (int32 stringIndex = 0; stringIndex < strings.Num(); ++stringIndex)
	{
		auto javaString = FJavaHelper::ToJavaString(javaEnvironment, strings[stringIndex]);
		javaEnvironment->SetObjectArrayElement(javaArray, stringIndex, *javaString);
	}

	return javaArray;
}
}  // namespace ApexLauncherJava
#endif

void UApexLauncherSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
	Super::Initialize(collection);
	UE_LOG(LogApexLauncher, Log, TEXT("Apex launcher started; mock downloader %s."),
		   UsesMockDownloader() ? TEXT("on") : TEXT("off"));
}

void UApexLauncherSubsystem::Deinitialize()
{
	OnPackageEventNative.Clear();
	Super::Deinitialize();
}

UApexLauncherSubsystem* UApexLauncherSubsystem::Get()
{
	return GEngine != nullptr ? GEngine->GetEngineSubsystem<UApexLauncherSubsystem>() : nullptr;
}

bool UApexLauncherSubsystem::IsApplicationInstalled(const FString& packageName) const
{
#if PLATFORM_ANDROID
	JNIEnv* javaEnvironment = FAndroidApplication::GetJavaEnv();
	if (javaEnvironment == nullptr)
	{
		return false;
	}

	auto javaPackageName = FJavaHelper::ToJavaString(javaEnvironment, packageName);
	return ApexLauncherJava::CallBoolean("AndroidThunkJava_PixoIsPackageInstalled", "(Ljava/lang/String;)Z",
										 *javaPackageName);
#else
	return false;
#endif
}

bool UApexLauncherSubsystem::LaunchApplication(const FString& packageName, const TArray<FString>& extraKeys,
											   const TArray<FString>& extraValues, bool quitAfterLaunch)
{
	if (extraKeys.Num() != extraValues.Num())
	{
		UE_LOG(LogApexLauncher, Error, TEXT("LaunchApplication: %d extra keys but %d values."), extraKeys.Num(),
			   extraValues.Num());
		return false;
	}

	UE_LOG(LogApexLauncher, Log, TEXT("Launching %s with %d extras."), *packageName, extraKeys.Num());
#if PLATFORM_ANDROID
	JNIEnv* javaEnvironment = FAndroidApplication::GetJavaEnv();
	if (javaEnvironment == nullptr)
	{
		return false;
	}

	auto javaPackageName = FJavaHelper::ToJavaString(javaEnvironment, packageName);
	auto javaExtraKeys =
		NewScopedJavaObject(javaEnvironment, ApexLauncherJava::MakeStringArray(javaEnvironment, extraKeys));
	auto javaExtraValues =
		NewScopedJavaObject(javaEnvironment, ApexLauncherJava::MakeStringArray(javaEnvironment, extraValues));
	return ApexLauncherJava::CallBoolean(
		"AndroidThunkJava_PixoLaunch", "(Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/String;Z)Z", *javaPackageName,
		*javaExtraKeys, *javaExtraValues, quitAfterLaunch ? JNI_TRUE : JNI_FALSE);
#else
	UE_LOG(LogApexLauncher, Warning, TEXT("Launching apps by package name only works on Android."));
	return false;
#endif
}

bool UApexLauncherSubsystem::LaunchUrl(const FString& url, bool quitAfterLaunch)
{
	UE_LOG(LogApexLauncher, Log, TEXT("Opening a URL."));
#if PLATFORM_ANDROID
	JNIEnv* javaEnvironment = FAndroidApplication::GetJavaEnv();
	if (javaEnvironment == nullptr)
	{
		return false;
	}

	auto javaUrl = FJavaHelper::ToJavaString(javaEnvironment, url);
	return ApexLauncherJava::CallBoolean("AndroidThunkJava_PixoOpenURL", "(Ljava/lang/String;Z)Z", *javaUrl,
										 quitAfterLaunch ? JNI_TRUE : JNI_FALSE);
#else
	FString errorText;
	FPlatformProcess::LaunchURL(*url, nullptr, &errorText);
	if (!errorText.IsEmpty())
	{
		UE_LOG(LogApexLauncher, Error, TEXT("Failed to open a URL: %s"), *errorText);
		return false;
	}

	if (quitAfterLaunch)
	{
		QuitApplication();
	}

	return true;
#endif
}

void UApexLauncherSubsystem::QuitApplication()
{
#if PLATFORM_ANDROID
	ApexLauncherJava::CallVoid("AndroidThunkJava_PixoQuit");
#else
	FPlatformMisc::RequestExit(false);
#endif
}

FString UApexLauncherSubsystem::GetIntentData() const
{
#if PLATFORM_ANDROID
	return ApexLauncherJava::CallString("AndroidThunkJava_GetIntentData", "()Ljava/lang/String;");
#else
	return FString();
#endif
}

FString UApexLauncherSubsystem::GetApplicationExtra(const FString& extraKey) const
{
#if PLATFORM_ANDROID
	JNIEnv* javaEnvironment = FAndroidApplication::GetJavaEnv();
	if (javaEnvironment == nullptr)
	{
		return FString();
	}

	auto javaExtraKey = FJavaHelper::ToJavaString(javaEnvironment, extraKey);
	return ApexLauncherJava::CallString("AndroidThunkJava_GetApplicationExtra",
										"(Ljava/lang/String;)Ljava/lang/String;", *javaExtraKey);
#else
	return FString();
#endif
}

FString UApexLauncherSubsystem::GetApplicationVersionName(const FString& packageName) const
{
#if PLATFORM_ANDROID
	JNIEnv* javaEnvironment = FAndroidApplication::GetJavaEnv();
	if (javaEnvironment == nullptr)
	{
		return FString();
	}

	auto javaPackageName = FJavaHelper::ToJavaString(javaEnvironment, packageName);
	return ApexLauncherJava::CallString("AndroidThunkJava_GetApplicationVersionName",
										"(Ljava/lang/String;)Ljava/lang/String;", *javaPackageName);
#else
	return FString();
#endif
}

bool UApexLauncherSubsystem::InstallApplication(UObject* worldContextObject, const FString& packageName,
												const FString& url)
{
	if (packageName.IsEmpty() || url.IsEmpty())
	{
		UE_LOG(LogApexLauncher, Error, TEXT("InstallApplication needs a package name and a URL."));
		return false;
	}

	if (!SupportsPackageInstalls())
	{
		UE_LOG(LogApexLauncher, Warning, TEXT("This headset type cannot install packages; %s was not installed."),
			   *packageName);
		return false;
	}

	UE_LOG(LogApexLauncher, Log, TEXT("Installing %s."), *packageName);
#if PLATFORM_ANDROID
	JNIEnv* javaEnvironment = FAndroidApplication::GetJavaEnv();
	if (javaEnvironment == nullptr)
	{
		return false;
	}

	auto javaPackageName = FJavaHelper::ToJavaString(javaEnvironment, packageName);
	auto javaUrl = FJavaHelper::ToJavaString(javaEnvironment, url);
	return ApexLauncherJava::CallBoolean("AndroidThunkJava_InstallApplication",
										 "(Ljava/lang/String;Ljava/lang/String;)Z", *javaPackageName, *javaUrl);
#else
	UWorld* world = worldContextObject != nullptr ? worldContextObject->GetWorld() : nullptr;
	if (world == nullptr || m_mockDownloaderClass == nullptr)
	{
		UE_LOG(LogApexLauncher, Warning, TEXT("Installing only works on Android; set a mock downloader to test."));
		return false;
	}

	AActor* mockDownloader = world->SpawnActor(m_mockDownloaderClass);
	OnMockDownloaderSpawned.Broadcast(mockDownloader, packageName, url);
	return mockDownloader != nullptr;
#endif
}

bool UApexLauncherSubsystem::UpdateApplication(UObject* worldContextObject, const FString& packageName,
											   const FString& url)
{
	return InstallApplication(worldContextObject, packageName, url);
}

bool UApexLauncherSubsystem::SupportsPackageInstalls() const
{
#if PLATFORM_ANDROID
	const EApexHeadsetType headsetType = GetDefault<UApexLauncherSettings>()->HeadsetType;
	return headsetType == EApexHeadsetType::PICO || headsetType == EApexHeadsetType::HTC_FOCUS;
#else
	return m_mockDownloaderClass != nullptr;
#endif
}

bool UApexLauncherSubsystem::UninstallApplication(const FString& packageName)
{
	if (!SupportsPackageInstalls())
	{
		UE_LOG(LogApexLauncher, Warning, TEXT("This headset type cannot uninstall packages; %s was not uninstalled."),
			   *packageName);
		return false;
	}

	UE_LOG(LogApexLauncher, Log, TEXT("Uninstalling %s."), *packageName);
#if PLATFORM_ANDROID
	JNIEnv* javaEnvironment = FAndroidApplication::GetJavaEnv();
	if (javaEnvironment == nullptr)
	{
		return false;
	}

	auto javaPackageName = FJavaHelper::ToJavaString(javaEnvironment, packageName);
	return ApexLauncherJava::CallBoolean("AndroidThunkJava_UninstallApplication", "(Ljava/lang/String;)Z",
										 *javaPackageName);
#else
	return false;
#endif
}

bool UApexLauncherSubsystem::IsUpdateAvailable(const FString& packageName, const FString& availableVersion) const
{
	if (!IsApplicationInstalled(packageName))
	{
		return true;
	}

	return CompareVersions(GetApplicationVersionName(packageName), availableVersion) < 0;
}

int32 UApexLauncherSubsystem::CompareVersions(const FString& firstVersion, const FString& secondVersion)
{
	TArray<FString> firstParts;
	TArray<FString> secondParts;
	firstVersion.ParseIntoArray(firstParts, TEXT("."));
	secondVersion.ParseIntoArray(secondParts, TEXT("."));

	const int32 partCount = FMath::Max(firstParts.Num(), secondParts.Num());
	for (int32 partIndex = 0; partIndex < partCount; ++partIndex)
	{
		const int64 firstNumber = firstParts.IsValidIndex(partIndex) ? FCString::Atoi64(*firstParts[partIndex]) : 0;
		const int64 secondNumber = secondParts.IsValidIndex(partIndex) ? FCString::Atoi64(*secondParts[partIndex]) : 0;
		if (firstNumber != secondNumber)
		{
			return firstNumber < secondNumber ? -1 : 1;
		}
	}

	return 0;
}

void UApexLauncherSubsystem::ReportPackageEvent(const FString& packageName, EApexPackageEvent packageEvent,
												float progressPercent, int64 sizeBytes)
{
	OnPackageEvent.Broadcast(packageName, packageEvent, progressPercent, sizeBytes);
	OnPackageEventNative.Broadcast(packageName, packageEvent, progressPercent, sizeBytes);
}

void UApexLauncherSubsystem::SetMockDownloaderClass(TSubclassOf<AActor> mockDownloaderClass)
{
	m_mockDownloaderClass = mockDownloaderClass;
}

bool UApexLauncherSubsystem::UsesMockDownloader() const
{
	return !PLATFORM_ANDROID;
}

void UApexLauncherSubsystem::LogInstalledPackages()
{
#if PLATFORM_ANDROID
	ApexLauncherJava::CallVoid("AndroidThunkJava_LogAllPackages");
#endif
}

bool UApexLauncherSubsystem::DoesSharedFileExist(const FString& fileName) const
{
#if PLATFORM_ANDROID
	JNIEnv* javaEnvironment = FAndroidApplication::GetJavaEnv();
	if (javaEnvironment == nullptr)
	{
		return false;
	}

	auto javaFileName = FJavaHelper::ToJavaString(javaEnvironment, fileName);
	return ApexLauncherJava::CallBoolean("AndroidThunkJava_FileExists", "(Ljava/lang/String;)Z", *javaFileName);
#else
	return false;
#endif
}

FString UApexLauncherSubsystem::WriteSharedFile(const FString& fileName, const FString& fileContent)
{
#if PLATFORM_ANDROID
	JNIEnv* javaEnvironment = FAndroidApplication::GetJavaEnv();
	if (javaEnvironment == nullptr)
	{
		return FString();
	}

	auto javaFileName = FJavaHelper::ToJavaString(javaEnvironment, fileName);
	auto javaFileContent = FJavaHelper::ToJavaString(javaEnvironment, fileContent);
	return ApexLauncherJava::CallString("AndroidThunkJava_WriteFileToSharedStorage",
										"(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;", *javaFileName,
										*javaFileContent);
#else
	return FString();
#endif
}

FString UApexLauncherSubsystem::ReadSharedFile(const FString& fileName) const
{
#if PLATFORM_ANDROID
	JNIEnv* javaEnvironment = FAndroidApplication::GetJavaEnv();
	if (javaEnvironment == nullptr)
	{
		return FString();
	}

	auto javaFileName = FJavaHelper::ToJavaString(javaEnvironment, fileName);
	return ApexLauncherJava::CallString("AndroidThunkJava_ReadFileFromSharedStorage",
										"(Ljava/lang/String;)Ljava/lang/String;", *javaFileName);
#else
	return FString();
#endif
}

bool UApexLauncherSubsystem::DeleteSharedFile(const FString& fileName)
{
#if PLATFORM_ANDROID
	JNIEnv* javaEnvironment = FAndroidApplication::GetJavaEnv();
	if (javaEnvironment == nullptr)
	{
		return false;
	}

	auto javaFileName = FJavaHelper::ToJavaString(javaEnvironment, fileName);
	return ApexLauncherJava::CallBoolean("AndroidThunkJava_DeleteFileFromSharedStorage", "(Ljava/lang/String;)Z",
										 *javaFileName);
#else
	return false;
#endif
}

FString UApexLauncherSubsystem::GetDeviceSerialNumber() const
{
#if PLATFORM_ANDROID
	return ApexLauncherJava::CallString("AndroidThunkJava_GetDeviceSerialNumber", "()Ljava/lang/String;");
#else
	return FString();
#endif
}

FString UApexLauncherSubsystem::GetDeviceMacAddress() const
{
#if PLATFORM_ANDROID
	return ApexLauncherJava::CallString("AndroidThunkJava_GetMacAddress", "()Ljava/lang/String;");
#else
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	return FPlatformMisc::GetMacAddressString();
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
#endif
}

bool UApexLauncherSubsystem::IsDeviceManaged() const
{
#if PLATFORM_ANDROID
	return ApexLauncherJava::CallBoolean("AndroidThunkJava_IsDeviceOwner", "()Z");
#else
	return false;
#endif
}
