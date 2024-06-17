// Copyright 2023 PixoVR Corp. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"

#if PLATFORM_ANDROID
#include "Android/AndroidJNI.h"
#include "Android/AndroidApplication.h"

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
	bool LaunchApplication(const FString& ApplicationName, const FString& ExtraKey, const FString& ExtraValue);

	UFUNCTION(BlueprintCallable, Category = "PixoVR|Launcher")
	FString GetApplicationExtra(const FString& ExtraKey);

#if PLATFORM_ANDROID
	jclass PlatformClassID;
	jmethodID LaunchApplicationFunction;
	jmethodID IsApplicationInstalledFunction;
	jmethodID GetApplicationExtraFunction;
#endif
};
