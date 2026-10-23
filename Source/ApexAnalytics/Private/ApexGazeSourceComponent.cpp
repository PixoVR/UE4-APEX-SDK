// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#include "ApexGazeSourceComponent.h"

#include "ApexAnalyticsSubsystem.h"

#include "EyeTrackerFunctionLibrary.h"
#include "EyeTrackerTypes.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

UApexGazeSourceComponent::UApexGazeSourceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UApexGazeSourceComponent::TryGetGaze_Implementation(FVector& /*gazeOrigin*/, FVector& /*gazeDirection*/,
														 FString& /*engagementName*/)
{
	return false;
}

bool UApexGazeSourceComponent::GetHeadTransform(FTransform& headTransform) const
{
	if (const UApexAnalyticsSubsystem* analyticsSubsystem = UApexAnalyticsSubsystem::Get(this))
	{
		if (analyticsSubsystem->GetHeadTransform(headTransform))
		{
			return true;
		}
	}

	if (GetOwner() != nullptr)
	{
		headTransform = GetOwner()->GetActorTransform();
		return true;
	}

	return false;
}

UApexHeadGazeSourceComponent::UApexHeadGazeSourceComponent() : m_engagementName(TEXT("gaze"))
{
}

bool UApexHeadGazeSourceComponent::TryGetGaze_Implementation(FVector& gazeOrigin, FVector& gazeDirection,
															 FString& engagementName)
{
	FTransform headTransform;
	if (!GetHeadTransform(headTransform))
	{
		return false;
	}

	gazeOrigin = headTransform.GetLocation();
	gazeDirection = headTransform.GetRotation().GetForwardVector();
	engagementName = m_engagementName;
	return true;
}

UApexEyeGazeSourceComponent::UApexEyeGazeSourceComponent() : m_engagementName(TEXT("eye_gaze")), m_fallBackToHead(true)
{
}

bool UApexEyeGazeSourceComponent::TryGetGaze_Implementation(FVector& gazeOrigin, FVector& gazeDirection,
															FString& engagementName)
{
	APlayerController* playerController = nullptr;
	if (const APawn* owningPawn = Cast<APawn>(GetOwner()))
	{
		playerController = Cast<APlayerController>(owningPawn->GetController());
	}

	if (playerController == nullptr && GetWorld() != nullptr)
	{
		playerController = GetWorld()->GetFirstPlayerController();
	}

	FEyeTrackerGazeData gazeData;
	if (UEyeTrackerFunctionLibrary::IsEyeTrackerConnected() &&
		UEyeTrackerFunctionLibrary::GetGazeData(gazeData, playerController) && gazeData.ConfidenceValue > 0.0f)
	{
		gazeOrigin = gazeData.GazeOrigin;
		gazeDirection = gazeData.GazeDirection;
		engagementName = m_engagementName;
		return true;
	}

	if (!m_fallBackToHead)
	{
		return false;
	}

	FTransform headTransform;
	if (!GetHeadTransform(headTransform))
	{
		return false;
	}

	gazeOrigin = headTransform.GetLocation();
	gazeDirection = headTransform.GetRotation().GetForwardVector();
	engagementName = TEXT("gaze");
	return true;
}
