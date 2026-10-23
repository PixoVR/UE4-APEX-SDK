// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ApexAnalyticsProvider.h"

class UApexAnalyticsSubsystem;
class UApexSubsystem;

/**
 * The built-in provider: forwards what the player does to apex-sdk-core, which records it in the
 * PixoVR telemetry format, batches it into packets and sends them where the settings say. Poses are
 * sampled whenever the core asks for them.
 */
class FPixoVRAnalyticsProvider : public IApexAnalyticsProvider
{
public:
	static constexpr const TCHAR* kProviderName = TEXT("PixoVR");

	FPixoVRAnalyticsProvider(UApexSubsystem& apexSubsystem, UApexAnalyticsSubsystem& analyticsSubsystem);

	virtual FString GetName() const override;
	virtual void OnTrackedObjectRegistered(UApexTrackedObjectComponent& trackedObject) override;
	virtual void OnTrackedObjectUnregistered(UApexTrackedObjectComponent& trackedObject) override;
	virtual void OnEngagementBegin(UApexTrackedObjectComponent& trackedObject, const FString& engagement) override;
	virtual void OnEngagementEnd(UApexTrackedObjectComponent& trackedObject, const FString& engagement) override;
	virtual void OnInteraction(UApexTrackedObjectComponent& trackedObject, const FString& action) override;
	virtual void OnStepBegin(const FApexAnalyticsContext& context, const FString& step) override;
	virtual void OnStepEnd(const FApexAnalyticsContext& context, const FString& step, bool succeeded,
						   float score) override;
	virtual void Tick(float deltaSeconds) override;
	virtual void Flush() override;

private:
	TWeakObjectPtr<UApexSubsystem> m_apexSubsystem;
	TWeakObjectPtr<UApexAnalyticsSubsystem> m_analyticsSubsystem;
};
