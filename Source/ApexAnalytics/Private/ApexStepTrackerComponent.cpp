// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#include "ApexStepTrackerComponent.h"

#include "ApexAnalyticsSubsystem.h"

UApexStepTrackerComponent::UApexStepTrackerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UApexStepTrackerComponent::Begin()
{
	if (UApexAnalyticsSubsystem* analyticsSubsystem = UApexAnalyticsSubsystem::Get(this))
	{
		analyticsSubsystem->BeginStep(m_stepName);
	}
}

void UApexStepTrackerComponent::Complete(float score)
{
	if (UApexAnalyticsSubsystem* analyticsSubsystem = UApexAnalyticsSubsystem::Get(this))
	{
		analyticsSubsystem->CompleteStep(m_stepName, score);
	}
}

void UApexStepTrackerComponent::Fail(float score)
{
	if (UApexAnalyticsSubsystem* analyticsSubsystem = UApexAnalyticsSubsystem::Get(this))
	{
		analyticsSubsystem->FailStep(m_stepName, score);
	}
}

FString UApexStepTrackerComponent::GetStepName() const
{
	return m_stepName;
}

void UApexStepTrackerComponent::SetStepName(const FString& stepName)
{
	m_stepName = stepName;
}
