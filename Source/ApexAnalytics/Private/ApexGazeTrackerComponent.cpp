// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#include "ApexGazeTrackerComponent.h"

#include "ApexGazeSourceComponent.h"
#include "ApexTrackedObjectComponent.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"

UApexGazeTrackerComponent::UApexGazeTrackerComponent()
	: m_maximumDistanceCentimeters(1000.0f),
	  m_dwellSeconds(0.5f),
	  m_sampleIntervalSeconds(0.1f),
	  m_traceChannel(ECC_Visibility),
	  m_source(nullptr),
	  m_targetSinceSeconds(0.0f),
	  m_isGazeActive(false)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UApexGazeTrackerComponent::BeginPlay()
{
	Super::BeginPlay();
	SetComponentTickInterval(FMath::Max(0.01f, m_sampleIntervalSeconds));
	ResolveSource();
}

void UApexGazeTrackerComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	ClearTarget();
	Super::EndPlay(endPlayReason);
}

void UApexGazeTrackerComponent::TickComponent(float deltaSeconds, ELevelTick tickType,
											  FActorComponentTickFunction* thisTickFunction)
{
	Super::TickComponent(deltaSeconds, tickType, thisTickFunction);
	Sample();
}

UApexTrackedObjectComponent* UApexGazeTrackerComponent::GetCurrentTarget() const
{
	return m_currentTarget.Get();
}

void UApexGazeTrackerComponent::Sample()
{
	ResolveSource();
	UWorld* world = GetWorld();
	FVector gazeOrigin;
	FVector gazeDirection;
	FString engagementName;
	if (world == nullptr || m_source == nullptr || !m_source->TryGetGaze(gazeOrigin, gazeDirection, engagementName) ||
		engagementName.IsEmpty())
	{
		ClearTarget();
		return;
	}

	FCollisionQueryParams queryParameters(SCENE_QUERY_STAT(ApexGazeTrace), true, GetOwner());
	FHitResult hitResult;
	const FVector traceEnd = gazeOrigin + gazeDirection.GetSafeNormal() * m_maximumDistanceCentimeters;
	UApexTrackedObjectComponent* target =
		world->LineTraceSingleByChannel(hitResult, gazeOrigin, traceEnd, m_traceChannel, queryParameters)
			? FindTrackedObject(hitResult)
			: nullptr;

	const float nowSeconds = world->GetTimeSeconds();
	if (target != m_currentTarget.Get() || !engagementName.Equals(m_currentEngagement, ESearchCase::CaseSensitive))
	{
		EndCurrentEngagement();
		m_currentTarget = target;
		m_currentEngagement = engagementName;
		m_targetSinceSeconds = nowSeconds;
		return;
	}

	if (target != nullptr && !m_isGazeActive && nowSeconds - m_targetSinceSeconds >= FMath::Max(0.0f, m_dwellSeconds))
	{
		target->BeginEngagement(engagementName);
		m_isGazeActive = true;
		m_activeEngagement = engagementName;
	}
}

void UApexGazeTrackerComponent::ResolveSource()
{
	AActor* owner = GetOwner();
	if (m_source != nullptr || owner == nullptr)
	{
		return;
	}

	m_source = owner->FindComponentByClass<UApexGazeSourceComponent>();
	if (m_source == nullptr)
	{
		m_source = NewObject<UApexHeadGazeSourceComponent>(owner, TEXT("ApexHeadGazeSource"));
		m_source->RegisterComponent();
	}
}

void UApexGazeTrackerComponent::EndCurrentEngagement()
{
	UApexTrackedObjectComponent* currentTarget = m_currentTarget.Get();
	if (currentTarget != nullptr && m_isGazeActive)
	{
		currentTarget->EndEngagement(m_activeEngagement);
	}

	m_isGazeActive = false;
	m_activeEngagement.Empty();
}

void UApexGazeTrackerComponent::ClearTarget()
{
	EndCurrentEngagement();
	m_currentTarget.Reset();
	m_currentEngagement.Empty();
}

UApexTrackedObjectComponent* UApexGazeTrackerComponent::FindTrackedObject(const FHitResult& hitResult) const
{
	// The hit actor, or the actor it is attached to, as Unity's GetComponentInParent.
	for (AActor* hitActor = hitResult.GetActor(); hitActor != nullptr; hitActor = hitActor->GetAttachParentActor())
	{
		if (UApexTrackedObjectComponent* trackedObject = hitActor->FindComponentByClass<UApexTrackedObjectComponent>())
		{
			return trackedObject;
		}
	}

	return nullptr;
}
