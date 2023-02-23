// Copyright 2022 PixoVR Corp. All Rights Reserved.
#pragma once

#include "ApexSDKSettings.generated.h"

UENUM(BlueprintType)
enum class EPVRVersionFields : uint8
{
	AndroidStoreVersion UMETA(DisplayName = "Android Store Version"),
	PixoVRPluginVersion UMETA(DisplayName = "PixoVR Plugin Version")
};

UCLASS(config = Engine, defaultconfig)
class APEXSDK_API UApexSDKSettings : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	/** Apex Endpoint */
	UPROPERTY(Config, EditAnywhere, Category = "Apex")
	FString ServerURI;

	/** Module ID */
	UPROPERTY(Config, EditAnywhere, Category = "Apex", meta = (DisplayName = "Module ID"))
	int ModuleId;

	/** Module major version */
	UPROPERTY(Config, EditAnywhere, Category = "Apex", meta=(DisplayName = "Module Major Version"))
	int MajorVerison;

	/** Module minor version */
	UPROPERTY(Config, EditAnywhere, Category = "Apex", meta=(DisplayName = "Module Minor Version"))
	int MinorVersion;

	/** Module patch version */
	UPROPERTY(Config, EditAnywhere, Category = "Apex", meta=(DisplayName = "Module Patch Version"))
	int PatchVersion;

	/** Sync versioning */
	UPROPERTY(Config, EditAnywhere, Category = "Apex", meta = (DisplayName = "Sync Module Version", Tooltip = "If sync is enabled, this will make at least the Android and Project version match the Module version. See Synchronized Version Fields"))
	bool bSyncVersion;

	/** Sync versioning */
	UPROPERTY(Config, EditAnywhere, Category = "Apex", meta = (DisplayName = "Additional Synchronized Version Fields", Tooltip = "Additional version fields that are synchronized with the module version."))
	TArray<TEnumAsByte<EPVRVersionFields>> AdditionalVersionFields;

#if WITH_EDITORONLY_DATA
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	virtual void UpdateVersion(const TArray<TEnumAsByte<EPVRVersionFields>>& Fields);
#endif
};
