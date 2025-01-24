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
		LaunchApplicationFunction = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_Launch", "(Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/String;)Z", false);
		OpenURLFunction = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_OpenURL", "(Ljava/lang/String;)V", false);
		QuitApplicationFunction = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_QuitApp", "()V", false);
		GetIntentDataFunction = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_GetIntentData", "()Ljava/lang/String;", false);
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

bool UPixoLauncherSubsystem::LaunchApplication(const FString& ApplicationName, const TArray<FString>& ExtraKey, const TArray<FString>& ExtraValue)
{
	bool bResult = false;
#if PLATFORM_ANDROID
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		auto ApplicationNameArg = FJavaHelper::ToJavaString(Env, ApplicationName);

		// Convert scope array into java fields
		auto ExtraKeyArg = NewScopedJavaObject(Env, (jobjectArray)Env->NewObjectArray(ExtraKey.Num(), FJavaWrapper::JavaStringClass, nullptr));
		auto ExtraValueArg = NewScopedJavaObject(Env, (jobjectArray)Env->NewObjectArray(ExtraKey.Num(), FJavaWrapper::JavaStringClass, nullptr));
		for (uint32 Param = 0; Param < ExtraKey.Num(); Param++)
		{
			auto KeyStringValue = FJavaHelper::ToJavaString(Env, ExtraKey[Param]);
			Env->SetObjectArrayElement(*ExtraKeyArg, Param, *KeyStringValue);

			auto ValueStringValue = FJavaHelper::ToJavaString(Env, ExtraValue[Param]);
			Env->SetObjectArrayElement(*ExtraValueArg, Param, *ValueStringValue);
		}
		bResult = FJavaWrapper::CallBooleanMethod(Env, FJavaWrapper::GameActivityThis, LaunchApplicationFunction, *ApplicationNameArg, *ExtraKeyArg, *ExtraValueArg);
	}

	Log("UPixoLauncherSubsystem::LaunchApplication: %s, %i : %i", *ApplicationName, ExtraKey.Num(), ExtraValue.Num());
#endif
	return bResult;
}

void UPixoLauncherSubsystem::OpenURL(const FString& URL)
{
#if PLATFORM_ANDROID
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		auto URLArg = FJavaHelper::ToJavaString(Env, URL);
		FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, OpenURLFunction, *URLArg);
	}

	Log("UPixoLauncherSubsystem::OpenURL: %s", *URL);
#endif
}

void UPixoLauncherSubsystem::QuitApplication()
{
#if PLATFORM_ANDROID
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, QuitApplicationFunction);
	}

	Log("UPixoLauncherSubsystem::QuitApplication");
#endif
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

FString UPixoLauncherSubsystem::GetIntentData()
{
	FString ReturnedExtraValue = FString("");
#if PLATFORM_ANDROID
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		auto JavaKeyValue = (jstring)FJavaWrapper::CallObjectMethod(Env, FJavaWrapper::GameActivityThis, GetIntentDataFunction);
		ReturnedExtraValue = FJavaHelper::FStringFromLocalRef(Env, JavaKeyValue);
	}

	Log("UPixoLauncherSubsystem::GetIntentData: %s", *ReturnedExtraValue);
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