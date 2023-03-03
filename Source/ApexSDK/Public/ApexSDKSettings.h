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
};
