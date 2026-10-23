// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "ApexSpatialSamplerComponent.generated.h"

/**
 * Marks the player's head for telemetry and gaze: attach it to the VR camera. Without one, the
 * first player's camera is used.
 */
UCLASS(ClassGroup = "Apex", meta = (BlueprintSpawnableComponent, DisplayName = "Apex Spatial Sampler"))
class APEXANALYTICS_API UApexSpatialSamplerComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
};
