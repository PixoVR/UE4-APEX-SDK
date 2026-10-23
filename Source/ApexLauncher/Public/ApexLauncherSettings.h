// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ApexLauncherSettings.generated.h"

/** The headset an Android build targets; it picks how packages are installed. */
UENUM(BlueprintType)
enum class EApexHeadsetType : uint8
{
	OTHER UMETA(DisplayName = "Other Android"),
	QUEST UMETA(DisplayName = "Meta Quest"),
	PICO UMETA(DisplayName = "Pico Neo 3/4"),
	HTC_FOCUS UMETA(DisplayName = "HTC Focus 3"),
	HTC_BUSINESS UMETA(DisplayName = "HTC Focus 3 - Business+"),
};

/**
 * Project Settings > Plugins > Apex Launcher. Read by the Android build (ApexLauncherAndroid_UPL.xml),
 * so a change applies to the next package. Stored in DefaultEngine.ini.
 */
UCLASS(Config = Engine, DefaultConfig, meta = (DisplayName = "Apex Launcher"))
class APEXLAUNCHER_API UApexLauncherSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UApexLauncherSettings();

	virtual FName GetCategoryName() const override;

	/** Picks the Java installer: the standard PackageInstaller, Pico's ToB service, or HTC's service. */
	UPROPERTY(Config, EditAnywhere, Category = "Android")
	EApexHeadsetType HeadsetType;

	/**
	 * Adds android:sharedUserId="android.uid.system" (not on Pico), which silent installs need on
	 * managed headsets. Only builds signed with the platform key install with it, so it is off by default.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Android", meta = (DisplayName = "Use System Shared User ID"))
	bool bUseSystemSharedUserId;
};
