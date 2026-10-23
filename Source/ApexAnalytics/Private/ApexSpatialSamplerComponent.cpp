// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#include "ApexSpatialSamplerComponent.h"

#include "ApexAnalyticsSubsystem.h"

void UApexSpatialSamplerComponent::BeginPlay()
{
	Super::BeginPlay();
	if (UApexAnalyticsSubsystem* analyticsSubsystem = UApexAnalyticsSubsystem::Get(this))
	{
		analyticsSubsystem->SetHeadComponent(this);
	}
}

void UApexSpatialSamplerComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	UApexAnalyticsSubsystem* analyticsSubsystem = UApexAnalyticsSubsystem::Get(this);
	if (analyticsSubsystem != nullptr && analyticsSubsystem->GetHeadComponent() == this)
	{
		analyticsSubsystem->SetHeadComponent(nullptr);
	}

	Super::EndPlay(endPlayReason);
}
