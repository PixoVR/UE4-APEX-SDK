// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "ApexGazeTrackerComponent.generated.h"

class UApexGazeSourceComponent;
class UApexTrackedObjectComponent;

/**
 * Records what the player looks at: traces along the gaze a few times a second, and when the gaze
 * stays on a tracked object for Dwell Seconds, begins a gaze engagement on it, ending it when the
 * gaze moves away. Add it to the player pawn; it uses the pawn's gaze source component, or head
 * gaze when there is none.
 */
UCLASS(ClassGroup = "Apex", meta = (BlueprintSpawnableComponent, DisplayName = "Apex Gaze Tracker"))
class APEXANALYTICS_API UApexGazeTrackerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UApexGazeTrackerComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
	virtual void TickComponent(float deltaSeconds, ELevelTick tickType,
							   FActorComponentTickFunction* thisTickFunction) override;

	/** The tracked object being looked at, engaged or not yet. */
	UFUNCTION(BlueprintPure, Category = "Apex|Analytics|Gaze")
	UApexTrackedObjectComponent* GetCurrentTarget() const;

	/** Traces now instead of waiting for the next sample. */
	UFUNCTION(BlueprintCallable, Category = "Apex|Analytics|Gaze")
	void Sample();

private:
	void ResolveSource();
	void EndCurrentEngagement();
	void ClearTarget();
	UApexTrackedObjectComponent* FindTrackedObject(const FHitResult& hitResult) const;

	UPROPERTY(EditAnywhere, Category = "Apex", meta = (DisplayName = "Maximum Distance (cm)", ClampMin = "1"))
	float m_maximumDistanceCentimeters;

	UPROPERTY(EditAnywhere, Category = "Apex", meta = (DisplayName = "Dwell Seconds", ClampMin = "0"))
	float m_dwellSeconds;

	UPROPERTY(EditAnywhere, Category = "Apex", meta = (DisplayName = "Sample Interval Seconds", ClampMin = "0.01"))
	float m_sampleIntervalSeconds;

	UPROPERTY(EditAnywhere, Category = "Apex", meta = (DisplayName = "Trace Channel"))
	TEnumAsByte<ECollisionChannel> m_traceChannel;

	UPROPERTY(Transient)
	UApexGazeSourceComponent* m_source;

	TWeakObjectPtr<UApexTrackedObjectComponent> m_currentTarget;
	FString m_currentEngagement;
	FString m_activeEngagement;
	float m_targetSinceSeconds;
	bool m_isGazeActive;
};
