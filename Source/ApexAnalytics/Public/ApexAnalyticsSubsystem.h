// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ApexAnalyticsProvider.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "ApexAnalyticsSubsystem.generated.h"

class USceneComponent;
class UApexSubsystem;
class UApexTrackedObjectComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FApexTrackedObjectDelegate, UApexTrackedObjectComponent*, TrackedObject);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FApexTrackedObjectActionDelegate, UApexTrackedObjectComponent*,
											 TrackedObject, const FString&, ActionName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FApexStepDelegate, const FString&, StepName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FApexStepEndDelegate, const FString&, StepName, bool, bSucceeded, float,
											   Score);

/**
 * The registered analytics providers and tracked objects, and the dispatch between them.
 *
 * Starts with the game instance. Unless Project Settings > Plugins > Apex SDK > Telemetry turns it
 * off, it registers the built-in PixoVR provider, which records everything in apex-sdk-core's
 * telemetry. Other providers (C++) register with RegisterProvider; Blueprints can bind the events
 * below to forward the same moments anywhere else.
 */
UCLASS(DisplayName = "Apex Analytics")
class APEXANALYTICS_API UApexAnalyticsSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	static UApexAnalyticsSubsystem* Get(const UObject* worldContextObject);

	virtual void Initialize(FSubsystemCollectionBase& collection) override;
	virtual void Deinitialize() override;

	virtual void Tick(float deltaSeconds) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;
	virtual bool IsTickableWhenPaused() const override;

	// ------------------------------------------------------------ Providers

	/** False when the name is taken or the settings disable analytics or this provider. */
	bool RegisterProvider(const TSharedRef<IApexAnalyticsProvider>& provider);
	bool UnregisterProvider(const TSharedRef<IApexAnalyticsProvider>& provider);
	const TArray<TSharedRef<IApexAnalyticsProvider>>& GetProviders() const;

	/** Calls every provider. */
	void Dispatch(TFunctionRef<void(IApexAnalyticsProvider& provider)> providerCall);

	UFUNCTION(BlueprintPure, Category = "Apex|Analytics")
	FApexAnalyticsContext MakeContext() const;

	/** Sends buffered telemetry now. Also happens when the app goes to the background. */
	UFUNCTION(BlueprintCallable, Category = "Apex|Analytics")
	void Flush();

	// ------------------------------------------------------------ Steps

	UFUNCTION(BlueprintCallable, Category = "Apex|Analytics|Steps")
	void BeginStep(const FString& stepName);

	UFUNCTION(BlueprintCallable, Category = "Apex|Analytics|Steps")
	void CompleteStep(const FString& stepName, float score = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Apex|Analytics|Steps")
	void FailStep(const FString& stepName, float score = 0.0f);

	// ------------------------------------------------------------ Tracked objects and the head

	UFUNCTION(BlueprintPure, Category = "Apex|Analytics")
	TArray<UApexTrackedObjectComponent*> GetTrackedObjects() const;

	/** The tracked object registered with this ID, or nullptr. */
	UApexTrackedObjectComponent* FindTrackedObject(const FString& trackedId) const;

	/** Called by UApexTrackedObjectComponent. */
	void RegisterTrackedObject(UApexTrackedObjectComponent& trackedObject);
	void UnregisterTrackedObject(UApexTrackedObjectComponent& trackedObject);
	void ReportEngagementBegin(UApexTrackedObjectComponent& trackedObject, const FString& engagement);
	void ReportEngagementEnd(UApexTrackedObjectComponent& trackedObject, const FString& engagement);
	void ReportInteraction(UApexTrackedObjectComponent& trackedObject, const FString& action);

	/** The component whose transform is the player's head (UApexSpatialSamplerComponent sets it). */
	UFUNCTION(BlueprintCallable, Category = "Apex|Analytics")
	void SetHeadComponent(USceneComponent* headComponent);

	UFUNCTION(BlueprintPure, Category = "Apex|Analytics")
	USceneComponent* GetHeadComponent() const;

	/** The head: the head component, else the first player's camera. False when there is neither. */
	UFUNCTION(BlueprintPure, Category = "Apex|Analytics")
	bool GetHeadTransform(FTransform& headTransform) const;

	// ------------------------------------------------------------ Events

	UPROPERTY(BlueprintAssignable, Category = "Apex|Analytics|Events")
	FApexTrackedObjectDelegate OnTrackedObjectRegistered;

	UPROPERTY(BlueprintAssignable, Category = "Apex|Analytics|Events")
	FApexTrackedObjectDelegate OnTrackedObjectUnregistered;

	UPROPERTY(BlueprintAssignable, Category = "Apex|Analytics|Events")
	FApexTrackedObjectActionDelegate OnEngagementBegan;

	UPROPERTY(BlueprintAssignable, Category = "Apex|Analytics|Events")
	FApexTrackedObjectActionDelegate OnEngagementEnded;

	UPROPERTY(BlueprintAssignable, Category = "Apex|Analytics|Events")
	FApexTrackedObjectActionDelegate OnInteraction;

	UPROPERTY(BlueprintAssignable, Category = "Apex|Analytics|Events")
	FApexStepDelegate OnStepBegan;

	UPROPERTY(BlueprintAssignable, Category = "Apex|Analytics|Events")
	FApexStepEndDelegate OnStepEnded;

private:
	void HandleUserIdentified(const FApexUser& user);
	void HandleSessionJoined(int64 sessionId);
	void HandleSessionEvent(const FString& action, const FString& targetObject, const FString& eventDataJson);
	void HandleSessionCompleted(const FApexSessionResult& sessionResult);
	void HandleApplicationBackground();
	bool IsStepNameValid(const FString& stepName, const TCHAR* actionName) const;

	TArray<TSharedRef<IApexAnalyticsProvider>> m_providers;
	TArray<TWeakObjectPtr<UApexTrackedObjectComponent>> m_trackedObjects;
	TWeakObjectPtr<USceneComponent> m_headComponent;
	TWeakObjectPtr<UApexSubsystem> m_apexSubsystem;
	FDelegateHandle m_backgroundDelegateHandle;
};
