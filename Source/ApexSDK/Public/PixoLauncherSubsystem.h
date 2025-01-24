// Copyright 2023 PixoVR Corp. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"

#if PLATFORM_ANDROID
#include "Android/AndroidJNI.h"
#include "Android/AndroidApplication.h"
#include "Android/AndroidPlatformMisc.h"

#include <android_native_app_glue.h>
#endif

#include "PixoLauncherSubsystem.generated.h"

/**
	\class UPixoLauncherSubsystem
	\brief A subsystem that contains functions to download, install and launch applications.

	UPixoLauncherSubsystem contains functionality to download, install and launch applications.

	\todo Create an interface class for each platform we want to use and establish a platform
		specific version of that class to call against.
 */
UCLASS(BlueprintType, Blueprintable, DisplayName = "PixoVR Launcher")
class UPixoLauncherSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	UPixoLauncherSubsystem();

	void Initialize(FSubsystemCollectionBase& Collection);

	UFUNCTION(BlueprintCallable, Category = "PixoVR|Launcher")
	bool IsApplicationInstalled(const FString& ApplicationName);
	
	UFUNCTION(BlueprintCallable, Category = "PixoVR|Launcher")
	bool LaunchApplication(const FString& ApplicationName, const TArray<FString>& ExtraKey, const TArray<FString>& ExtraValue);

	UFUNCTION(BlueprintCallable, Category = "PixoVR|Launcher")
	void OpenURL(const FString& URL);

	UFUNCTION(BlueprintCallable, Category = "PixoVR|Launcher")
	void QuitApplication();

	UFUNCTION(BlueprintCallable, Category = "PixoVR|Launcher")
	FString GetIntentData();

	UFUNCTION(BlueprintCallable, Category = "PixoVR|Launcher")
	FString GetApplicationExtra(const FString& ExtraKey);

	UFUNCTION(BlueprintCallable, Category = "PixoVR|Launcher")
	FString GetDeviceSerialNumber();

	UFUNCTION(BlueprintCallable, Category = "PixoVR|Launcher")
	FString GetDeviceMacAddress();

#if PLATFORM_ANDROID
	jclass PlatformClassID;
	jmethodID LaunchApplicationFunction;
	jmethodID OpenURLFunction;
	jmethodID QuitApplicationFunction;
	jmethodID IsApplicationInstalledFunction;
	jmethodID GetIntentDataFunction;
	jmethodID GetApplicationExtraFunction;
	jmethodID GetDeviceSerialNumberFunction;
	jmethodID GetDeviceMacAddressFunction;
#endif
};
