// Copyright 2022 PixoVR Corp. All Rights Reserved.
#pragma once

#include "ApexSDKSettings.generated.h"

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

	/** Module patch version */
	UPROPERTY(Config, EditAnywhere, Category = "Apex", meta = (DisplayName = "Sync Module Version", Tooltip = "If sync is enabled, this will make Android and Project version match the Module version."))
	bool bSyncVersion;

#if WITH_EDITORONLY_DATA
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	virtual void UpdateVersion();
#endif
};
