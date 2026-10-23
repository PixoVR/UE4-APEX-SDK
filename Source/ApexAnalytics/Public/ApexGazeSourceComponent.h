// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ApexGazeSourceComponent.generated.h"

/**
 * Where the player is looking, for UApexGazeTrackerComponent. Subclass it (C++ or Blueprint) for
 * another gaze input; the head and eye-tracker sources below cover the usual cases.
 */
UCLASS(Abstract, Blueprintable, ClassGroup = "Apex")
class APEXANALYTICS_API UApexGazeSourceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UApexGazeSourceComponent();

	/** False when there is no gaze this frame; the tracker then ends the current engagement. */
	UFUNCTION(BlueprintNativeEvent, Category = "Apex|Analytics|Gaze")
	bool TryGetGaze(FVector& gazeOrigin, FVector& gazeDirection, FString& engagementName);
	virtual bool TryGetGaze_Implementation(FVector& gazeOrigin, FVector& gazeDirection, FString& engagementName);

protected:
	/** The head: the analytics subsystem's head, else the first player's camera, else the owner. */
	bool GetHeadTransform(FTransform& headTransform) const;
};

/** Gaze along the head's forward direction, reported as the "gaze" engagement. */
UCLASS(ClassGroup = "Apex", meta = (BlueprintSpawnableComponent, DisplayName = "Apex Head Gaze Source"))
class APEXANALYTICS_API UApexHeadGazeSourceComponent : public UApexGazeSourceComponent
{
	GENERATED_BODY()

public:
	UApexHeadGazeSourceComponent();

	virtual bool TryGetGaze_Implementation(FVector& gazeOrigin, FVector& gazeDirection,
										   FString& engagementName) override;

private:
	UPROPERTY(EditAnywhere, Category = "Apex", meta = (DisplayName = "Engagement Name"))
	FString m_engagementName;
};

/**
 * Gaze from the headset's eye tracker (Unreal's EyeTracker interface, e.g. OpenXR eye gaze), reported
 * as "eye_gaze". Without eye tracking it falls back to head gaze ("gaze") when allowed.
 */
UCLASS(ClassGroup = "Apex", meta = (BlueprintSpawnableComponent, DisplayName = "Apex Eye Gaze Source"))
class APEXANALYTICS_API UApexEyeGazeSourceComponent : public UApexGazeSourceComponent
{
	GENERATED_BODY()

public:
	UApexEyeGazeSourceComponent();

	virtual bool TryGetGaze_Implementation(FVector& gazeOrigin, FVector& gazeDirection,
										   FString& engagementName) override;

private:
	UPROPERTY(EditAnywhere, Category = "Apex", meta = (DisplayName = "Engagement Name"))
	FString m_engagementName;

	UPROPERTY(EditAnywhere, Category = "Apex", meta = (DisplayName = "Fall Back To Head"))
	bool m_fallBackToHead;
};
