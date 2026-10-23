// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#include "PixoVRAnalyticsProvider.h"

#include "ApexAnalyticsSubsystem.h"
#include "ApexSubsystem.h"
#include "ApexTrackedObjectComponent.h"

FPixoVRAnalyticsProvider::FPixoVRAnalyticsProvider(UApexSubsystem& apexSubsystem,
												   UApexAnalyticsSubsystem& analyticsSubsystem)
	: m_apexSubsystem(&apexSubsystem), m_analyticsSubsystem(&analyticsSubsystem)
{
}

FString FPixoVRAnalyticsProvider::GetName() const
{
	return kProviderName;
}

void FPixoVRAnalyticsProvider::OnTrackedObjectRegistered(UApexTrackedObjectComponent& trackedObject)
{
	if (UApexSubsystem* apexSubsystem = m_apexSubsystem.Get())
	{
		apexSubsystem->TelemetryObjectRegistered(trackedObject.GetTrackedId(), trackedObject.GetTrackedDisplayName(),
												 trackedObject.GetMeshName(), trackedObject.GetTrackedTransform());
	}
}

void FPixoVRAnalyticsProvider::OnTrackedObjectUnregistered(UApexTrackedObjectComponent& trackedObject)
{
	if (UApexSubsystem* apexSubsystem = m_apexSubsystem.Get())
	{
		apexSubsystem->TelemetryObjectUnregistered(trackedObject.GetTrackedId(), trackedObject.GetTrackedTransform());
	}
}

void FPixoVRAnalyticsProvider::OnEngagementBegin(UApexTrackedObjectComponent& trackedObject, const FString& engagement)
{
	if (UApexSubsystem* apexSubsystem = m_apexSubsystem.Get())
	{
		apexSubsystem->TelemetryEngagementBegin(trackedObject.GetTrackedId(), engagement);
	}
}

void FPixoVRAnalyticsProvider::OnEngagementEnd(UApexTrackedObjectComponent& trackedObject, const FString& engagement)
{
	if (UApexSubsystem* apexSubsystem = m_apexSubsystem.Get())
	{
		apexSubsystem->TelemetryEngagementEnd(trackedObject.GetTrackedId(), engagement);
	}
}

void FPixoVRAnalyticsProvider::OnInteraction(UApexTrackedObjectComponent& trackedObject, const FString& action)
{
	if (UApexSubsystem* apexSubsystem = m_apexSubsystem.Get())
	{
		apexSubsystem->TelemetryInteraction(trackedObject.GetTrackedId(), action);
	}
}

void FPixoVRAnalyticsProvider::OnStepBegin(const FApexAnalyticsContext& /*context*/, const FString& step)
{
	if (UApexSubsystem* apexSubsystem = m_apexSubsystem.Get())
	{
		apexSubsystem->TelemetryStepBegin(step);
	}
}

void FPixoVRAnalyticsProvider::OnStepEnd(const FApexAnalyticsContext& /*context*/, const FString& step, bool succeeded,
										 float score)
{
	if (UApexSubsystem* apexSubsystem = m_apexSubsystem.Get())
	{
		apexSubsystem->TelemetryStepEnd(step, succeeded, score);
	}
}

void FPixoVRAnalyticsProvider::Tick(float /*deltaSeconds*/)
{
	UApexSubsystem* apexSubsystem = m_apexSubsystem.Get();
	UApexAnalyticsSubsystem* analyticsSubsystem = m_analyticsSubsystem.Get();
	if (apexSubsystem == nullptr || analyticsSubsystem == nullptr || !apexSubsystem->IsTelemetryPoseDue())
	{
		return;
	}

	FTransform headTransform;
	if (analyticsSubsystem->GetHeadTransform(headTransform))
	{
		apexSubsystem->TelemetryPose(FString(), headTransform);
	}

	for (UApexTrackedObjectComponent* trackedObject : analyticsSubsystem->GetTrackedObjects())
	{
		if (trackedObject->GetTrackPose())
		{
			apexSubsystem->TelemetryPose(trackedObject->GetTrackedId(), trackedObject->GetTrackedTransform());
		}
	}
}

void FPixoVRAnalyticsProvider::Flush()
{
	if (UApexSubsystem* apexSubsystem = m_apexSubsystem.Get())
	{
		apexSubsystem->TelemetryFlush();
	}
}
