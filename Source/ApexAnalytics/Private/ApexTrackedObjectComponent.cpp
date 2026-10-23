// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#include "ApexTrackedObjectComponent.h"

#include "ApexAnalyticsSubsystem.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY_STATIC(LogApexTrackedObject, Log, All);

UApexTrackedObjectComponent::UApexTrackedObjectComponent() : m_trackPose(false), m_isRegistered(false)
{
	PrimaryComponentTick.bCanEverTick = false;
}

FString UApexTrackedObjectComponent::GetTrackedId() const
{
	return m_trackedId;
}

void UApexTrackedObjectComponent::SetTrackedId(const FString& newTrackedId)
{
	if (newTrackedId.IsEmpty())
	{
		UE_LOG(LogApexTrackedObject, Warning, TEXT("A tracked ID cannot be empty."));
		return;
	}

	UApexAnalyticsSubsystem* analyticsSubsystem = UApexAnalyticsSubsystem::Get(this);
	const bool wasRegistered = m_isRegistered && analyticsSubsystem != nullptr;
	if (wasRegistered)
	{
		analyticsSubsystem->UnregisterTrackedObject(*this);
	}

	m_trackedId = newTrackedId;
	EnsureUniqueTrackedId();
	if (wasRegistered)
	{
		analyticsSubsystem->RegisterTrackedObject(*this);
	}
}

FString UApexTrackedObjectComponent::GetTrackedDisplayName() const
{
	if (!m_displayName.IsEmpty() || GetOwner() == nullptr)
	{
		return m_displayName;
	}

	return GetOwner()->GetName();
}

FString UApexTrackedObjectComponent::GetMeshName() const
{
	const AActor* owner = GetOwner();
	if (!m_meshName.IsEmpty() || owner == nullptr)
	{
		return m_meshName;
	}

	if (const UStaticMeshComponent* staticMeshComponent = owner->FindComponentByClass<UStaticMeshComponent>())
	{
		if (staticMeshComponent->GetStaticMesh() != nullptr)
		{
			return staticMeshComponent->GetStaticMesh()->GetName();
		}
	}

	if (const USkeletalMeshComponent* skeletalMeshComponent = owner->FindComponentByClass<USkeletalMeshComponent>())
	{
		if (skeletalMeshComponent->SkeletalMesh != nullptr)
		{
			return skeletalMeshComponent->SkeletalMesh->GetName();
		}
	}

	return FString();
}

bool UApexTrackedObjectComponent::GetTrackPose() const
{
	return m_trackPose;
}

void UApexTrackedObjectComponent::SetTrackPose(bool trackPose)
{
	m_trackPose = trackPose;
}

FTransform UApexTrackedObjectComponent::GetTrackedTransform() const
{
	return GetOwner() != nullptr ? GetOwner()->GetActorTransform() : FTransform::Identity;
}

void UApexTrackedObjectComponent::BeginEngagement(const FString& engagement)
{
	if (engagement.IsEmpty())
	{
		UE_LOG(LogApexTrackedObject, Warning, TEXT("Cannot begin an empty engagement."));
		return;
	}

	if (UApexAnalyticsSubsystem* analyticsSubsystem = UApexAnalyticsSubsystem::Get(this))
	{
		analyticsSubsystem->ReportEngagementBegin(*this, engagement);
	}
}

void UApexTrackedObjectComponent::EndEngagement(const FString& engagement)
{
	if (engagement.IsEmpty())
	{
		UE_LOG(LogApexTrackedObject, Warning, TEXT("Cannot end an empty engagement."));
		return;
	}

	if (UApexAnalyticsSubsystem* analyticsSubsystem = UApexAnalyticsSubsystem::Get(this))
	{
		analyticsSubsystem->ReportEngagementEnd(*this, engagement);
	}
}

void UApexTrackedObjectComponent::RecordInteraction(const FString& action)
{
	if (action.IsEmpty())
	{
		UE_LOG(LogApexTrackedObject, Warning, TEXT("Cannot record an empty interaction."));
		return;
	}

	if (UApexAnalyticsSubsystem* analyticsSubsystem = UApexAnalyticsSubsystem::Get(this))
	{
		analyticsSubsystem->ReportInteraction(*this, action);
	}
}

void UApexTrackedObjectComponent::OnComponentCreated()
{
	Super::OnComponentCreated();
	EnsureTrackedId();
}

void UApexTrackedObjectComponent::PostDuplicate(bool duplicateForPie)
{
	Super::PostDuplicate(duplicateForPie);
	if (!duplicateForPie)
	{
		// A copy in the editor is a different object.
		m_trackedId = FGuid::NewGuid().ToString(EGuidFormats::Digits).ToLower();
	}
}

void UApexTrackedObjectComponent::BeginPlay()
{
	Super::BeginPlay();
	EnsureUniqueTrackedId();
	if (UApexAnalyticsSubsystem* analyticsSubsystem = UApexAnalyticsSubsystem::Get(this))
	{
		analyticsSubsystem->RegisterTrackedObject(*this);
		m_isRegistered = true;
	}
}

void UApexTrackedObjectComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	if (m_isRegistered)
	{
		if (UApexAnalyticsSubsystem* analyticsSubsystem = UApexAnalyticsSubsystem::Get(this))
		{
			analyticsSubsystem->UnregisterTrackedObject(*this);
		}

		m_isRegistered = false;
	}

	Super::EndPlay(endPlayReason);
}

void UApexTrackedObjectComponent::EnsureTrackedId()
{
	if (m_trackedId.IsEmpty())
	{
		m_trackedId = FGuid::NewGuid().ToString(EGuidFormats::Digits).ToLower();
	}
}

void UApexTrackedObjectComponent::EnsureUniqueTrackedId()
{
	EnsureTrackedId();
	UApexAnalyticsSubsystem* analyticsSubsystem = UApexAnalyticsSubsystem::Get(this);
	UApexTrackedObjectComponent* sameIdObject =
		analyticsSubsystem != nullptr ? analyticsSubsystem->FindTrackedObject(m_trackedId) : nullptr;
	if (sameIdObject != nullptr && sameIdObject != this)
	{
		// Spawned copies of a Blueprint share its default ID.
		m_trackedId = FGuid::NewGuid().ToString(EGuidFormats::Digits).ToLower();
	}
}
