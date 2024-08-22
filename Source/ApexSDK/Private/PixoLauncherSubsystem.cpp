// Copyright 2023 PixoVR Corp. All Rights Reserved.


#include "PixoLauncherSubsystem.h"
#include "GenericPlatform/GenericPlatformMisc.h"

DEFINE_LOG_CATEGORY_STATIC(LogPixoLauncherSubsystem, Log, All);

#define Log(pmt, ...)	UE_LOG(LogPixoLauncherSubsystem, Log, TEXT(pmt), ##__VA_ARGS__)
#define Warn(pmt, ...)	UE_LOG(LogPixoLauncherSubsystem, Warning, TEXT(pmt), ##__VA_ARGS__)
#define Error(pmt, ...) UE_LOG(LogPixoLauncherSubsystem, Error, TEXT(pmt), ##__VA_ARGS__)
#define Fatal(pmt, ...) UE_LOG(LogPixoLauncherSubsystem, Fatal, TEXT(pmt), ##__VA_ARGS__)

UPixoLauncherSubsystem::UPixoLauncherSubsystem()
	: UEngineSubsystem()
{

}

void UPixoLauncherSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
#if PLATFORM_ANDROID
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		IsApplicationInstalledFunction = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_IsPackageInstalled", "(Ljava/lang/String;)Z", false);
		LaunchApplicationFunction = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_Launch", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Z", false);
		GetApplicationExtraFunction = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_GetApplicationExtra", "(Ljava/lang/String;)Ljava/lang/String;", false);
		GetDeviceSerialNumberFunction = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_GetDeviceSerialNumber", "()Ljava/lang/String;", false);
		GetDeviceMacAddressFunction = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_GetMacAddress", "()Ljava/lang/String;", false);
	}
#endif
}

bool UPixoLauncherSubsystem::IsApplicationInstalled(const FString& ApplicationName)
{
	bool bResult = false;
#if PLATFORM_ANDROID
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		auto PackageNameArg = FJavaHelper::ToJavaString(Env, ApplicationName);
		bResult = FJavaWrapper::CallBooleanMethod(Env, FJavaWrapper::GameActivityThis, FJavaWrapper::AndroidThunkJava_IsPackageInstalled, *PackageNameArg);
	}

	Log("UPixoLauncherSubsystem::IsApplicationInstalledFunction: %d", bResult);
#endif
	return bResult;
}

bool UPixoLauncherSubsystem::LaunchApplication(const FString& ApplicationName, const FString& ExtraKey, const FString& ExtraValue)
{
	bool bResult = false;
#if PLATFORM_ANDROID
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		auto ApplicationNameArg = FJavaHelper::ToJavaString(Env, ApplicationName);
		auto ExtraKeyArg = FJavaHelper::ToJavaString(Env, ExtraKey);
		auto ExtraValueArg = FJavaHelper::ToJavaString(Env, ExtraValue);
		bResult = FJavaWrapper::CallBooleanMethod(Env, FJavaWrapper::GameActivityThis, LaunchApplicationFunction, *ApplicationNameArg, *ExtraKeyArg, *ExtraValueArg);
	}

	Log("UPixoLauncherSubsystem::LaunchApplication: %s, %s : %s", *ApplicationName, *ExtraKey, *ExtraValue);
#endif
	return bResult;
}

FString UPixoLauncherSubsystem::GetApplicationExtra(const FString& ExtraKey)
{
	FString ReturnedExtraValue = FString("");
#if PLATFORM_ANDROID
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		auto ExtraKeyArg = FJavaHelper::ToJavaString(Env, ExtraKey);
		auto JavaKeyValue = (jstring)FJavaWrapper::CallObjectMethod(Env, FJavaWrapper::GameActivityThis, GetApplicationExtraFunction, *ExtraKeyArg);
		ReturnedExtraValue = FJavaHelper::FStringFromLocalRef(Env, JavaKeyValue);
	}

	Log("UPixoLauncherSubsystem::GetApplicationExtra: %s", *ReturnedExtraValue);
#endif
	return ReturnedExtraValue;
}

FString UPixoLauncherSubsystem::GetDeviceSerialNumber()
{
	FString ReturnedExtraValue = FString("");
#if PLATFORM_ANDROID
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		auto JavaKeyValue = (jstring)FJavaWrapper::CallObjectMethod(Env, FJavaWrapper::GameActivityThis, GetDeviceSerialNumberFunction);
		ReturnedExtraValue = FJavaHelper::FStringFromLocalRef(Env, JavaKeyValue);
	}

	Log("UPixoLauncherSubsystem::GetDeviceSerialNumber: %s", *ReturnedExtraValue);
#else
	ReturnedExtraValue = TEXT("");
#endif
	return ReturnedExtraValue;
}

FString UPixoLauncherSubsystem::GetDeviceMacAddress()
{
	FString ReturnedExtraValue = FString("");
#if PLATFORM_ANDROID
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		auto JavaKeyValue = (jstring)FJavaWrapper::CallObjectMethod(Env, FJavaWrapper::GameActivityThis, GetDeviceMacAddressFunction);
		ReturnedExtraValue = FJavaHelper::FStringFromLocalRef(Env, JavaKeyValue);
	}

	Log("UPixoLauncherSubsystem::GetDeviceMacAddress: %s", *ReturnedExtraValue);
#else
	ReturnedExtraValue = FGenericPlatformMisc::GetMacAddressString();
#endif
	return ReturnedExtraValue;
}

#undef Fatal
#undef Error
#undef Warn
#undef Log