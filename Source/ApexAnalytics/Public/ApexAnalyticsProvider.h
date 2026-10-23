// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ApexTypes.h"
#include "ApexAnalyticsProvider.generated.h"

class UApexTrackedObjectComponent;

/** Who is playing what, passed to analytics providers with each event. */
USTRUCT(BlueprintType)
struct APEXANALYTICS_API FApexAnalyticsContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Analytics")
	int32 UserId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Analytics")
	int32 OrganizationId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Analytics")
	FString UserEmail;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Analytics")
	int32 ModuleId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Analytics")
	FString ModuleName;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Analytics")
	FString ModuleVersion;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Analytics")
	FString ScenarioId;

	/** The platform's session ID, or 0 outside a session. */
	UPROPERTY(BlueprintReadOnly, Category = "Apex|Analytics")
	int64 SessionId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Analytics")
	FString DeviceId;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Analytics")
	FString DeviceModel;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Analytics")
	FString DeviceSerial;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Analytics")
	FString Platform;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Analytics")
	FString SdkVersion;
};

/**
 * Receives what the player does. Register one with UApexAnalyticsSubsystem::RegisterProvider; every
 * method has an empty default, so a provider implements only what it needs. Calls arrive on the
 * game thread. Blueprints can listen to the same moments through the subsystem's events.
 */
class APEXANALYTICS_API IApexAnalyticsProvider
{
public:
	virtual ~IApexAnalyticsProvider() = default;

	/** Unique; the settings' Disabled Providers list names providers by it. */
	virtual FString GetName() const = 0;

	virtual void OnUserIdentified(const FApexAnalyticsContext& context, const FApexUser& user)
	{
	}

	virtual void OnSessionJoined(const FApexAnalyticsContext& context, int64 sessionId)
	{
	}

	/** eventDataJson is the module's own data sent with the event, or empty. */
	virtual void OnSessionEvent(const FApexAnalyticsContext& context, const FString& action,
								const FString& targetObject, const FString& eventDataJson)
	{
	}

	virtual void OnSessionCompleted(const FApexAnalyticsContext& context, const FApexSessionResult& sessionResult)
	{
	}

	virtual void OnTrackedObjectRegistered(UApexTrackedObjectComponent& trackedObject)
	{
	}

	virtual void OnTrackedObjectUnregistered(UApexTrackedObjectComponent& trackedObject)
	{
	}

	virtual void OnEngagementBegin(UApexTrackedObjectComponent& trackedObject, const FString& engagement)
	{
	}

	virtual void OnEngagementEnd(UApexTrackedObjectComponent& trackedObject, const FString& engagement)
	{
	}

	virtual void OnInteraction(UApexTrackedObjectComponent& trackedObject, const FString& action)
	{
	}

	virtual void OnStepBegin(const FApexAnalyticsContext& context, const FString& step)
	{
	}

	virtual void OnStepEnd(const FApexAnalyticsContext& context, const FString& step, bool succeeded, float score)
	{
	}

	/** Called every frame; the built-in provider samples poses here. */
	virtual void Tick(float deltaSeconds)
	{
	}

	/** Send anything buffered, e.g. because the app is going to the background. */
	virtual void Flush()
	{
	}
};
