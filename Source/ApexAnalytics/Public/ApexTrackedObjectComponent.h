// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ApexTrackedObjectComponent.generated.h"

/**
 * Makes its actor a tracked object in Apex telemetry: registered while it plays, with engagements
 * (gaze, hover, grab) and interactions recorded against it, and its pose sampled when Track Pose is on.
 *
 * Add it to any actor. The tracked ID is kept across runs for actors placed in a level; spawned and
 * duplicated actors get a new one.
 */
UCLASS(ClassGroup = "Apex", meta = (BlueprintSpawnableComponent, DisplayName = "Apex Tracked Object"))
class APEXANALYTICS_API UApexTrackedObjectComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UApexTrackedObjectComponent();

	UFUNCTION(BlueprintPure, Category = "Apex|Analytics")
	FString GetTrackedId() const;

	/** Changes the ID; a registered object is re-registered under it. */
	UFUNCTION(BlueprintCallable, Category = "Apex|Analytics")
	void SetTrackedId(const FString& newTrackedId);

	/** The display name, or the actor's name when none is set. */
	UFUNCTION(BlueprintPure, Category = "Apex|Analytics")
	FString GetTrackedDisplayName() const;

	/** The mesh name, or the actor's first static or skeletal mesh when none is set. */
	UFUNCTION(BlueprintPure, Category = "Apex|Analytics")
	FString GetMeshName() const;

	UFUNCTION(BlueprintPure, Category = "Apex|Analytics")
	bool GetTrackPose() const;

	UFUNCTION(BlueprintCallable, Category = "Apex|Analytics")
	void SetTrackPose(bool trackPose);

	/** The actor's transform, which telemetry records. */
	UFUNCTION(BlueprintPure, Category = "Apex|Analytics")
	FTransform GetTrackedTransform() const;

	/** Starts an engagement, e.g. "gaze", "hover" or "grab", that lasts until EndEngagement. */
	UFUNCTION(BlueprintCallable, Category = "Apex|Analytics")
	void BeginEngagement(const FString& engagement);

	UFUNCTION(BlueprintCallable, Category = "Apex|Analytics")
	void EndEngagement(const FString& engagement);

	/** A one-off action on the object, e.g. "use" or "press". */
	UFUNCTION(BlueprintCallable, Category = "Apex|Analytics")
	void RecordInteraction(const FString& action);

	virtual void OnComponentCreated() override;
	virtual void PostDuplicate(bool duplicateForPie) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

private:
	void EnsureTrackedId();
	void EnsureUniqueTrackedId();

	UPROPERTY(EditAnywhere, Category = "Apex", meta = (DisplayName = "Tracked ID"))
	FString m_trackedId;

	UPROPERTY(EditAnywhere, Category = "Apex", meta = (DisplayName = "Display Name"))
	FString m_displayName;

	UPROPERTY(EditAnywhere, Category = "Apex", meta = (DisplayName = "Mesh Name"))
	FString m_meshName;

	/** Samples the actor's pose with the head's, a few times a second (Pose Samples Per Second). */
	UPROPERTY(EditAnywhere, Category = "Apex", meta = (DisplayName = "Track Pose"))
	bool m_trackPose;

	bool m_isRegistered;
};
