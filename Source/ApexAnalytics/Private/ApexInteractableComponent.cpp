// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#include "ApexInteractableComponent.h"

#include "ApexSubsystem.h"

UApexInteractableComponent::UApexInteractableComponent()
	: m_sendSessionEvents(false), m_isGrabbed(false), m_isHovered(false)
{
}

void UApexInteractableComponent::Grab()
{
	if (m_isGrabbed)
	{
		return;
	}

	m_isGrabbed = true;
	BeginEngagement(TEXT("grab"));
	SendSessionEvent(TEXT("grab"));
}

void UApexInteractableComponent::Release()
{
	if (!m_isGrabbed)
	{
		return;
	}

	m_isGrabbed = false;
	EndEngagement(TEXT("grab"));
	SendSessionEvent(TEXT("release"));
}

void UApexInteractableComponent::HoverEnter()
{
	if (m_isHovered)
	{
		return;
	}

	m_isHovered = true;
	BeginEngagement(TEXT("hover"));
}

void UApexInteractableComponent::HoverExit()
{
	if (!m_isHovered)
	{
		return;
	}

	m_isHovered = false;
	EndEngagement(TEXT("hover"));
}

void UApexInteractableComponent::Use()
{
	RecordInteraction(TEXT("use"));
	SendSessionEvent(TEXT("use"));
}

bool UApexInteractableComponent::IsGrabbed() const
{
	return m_isGrabbed;
}

bool UApexInteractableComponent::IsHovered() const
{
	return m_isHovered;
}

void UApexInteractableComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	// Engagements still open end with the object.
	if (m_isGrabbed)
	{
		m_isGrabbed = false;
		EndEngagement(TEXT("grab"));
	}

	if (m_isHovered)
	{
		m_isHovered = false;
		EndEngagement(TEXT("hover"));
	}

	Super::EndPlay(endPlayReason);
}

void UApexInteractableComponent::SendSessionEvent(const FString& action)
{
	UApexSubsystem* apexSubsystem = m_sendSessionEvents ? UApexSubsystem::Get(this) : nullptr;
	if (apexSubsystem != nullptr && apexSubsystem->IsSessionInProgress())
	{
		apexSubsystem->SendSessionEvent(action, GetTrackedDisplayName(), FString());
	}
}
