// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#include "ApexSessionComponent.h"

#include "ApexSubsystem.h"

UApexSessionComponent::UApexSessionComponent()
	: m_joinOnBeginPlay(true), m_loginWithLaunchToken(true), m_completeOnEndPlay(true)
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UApexSessionComponent::BeginPlay()
{
	Super::BeginPlay();
	if (m_joinOnBeginPlay)
	{
		StartSession();
	}
}

void UApexSessionComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	UApexSubsystem* apexSubsystem = UApexSubsystem::Get(this);
	if (m_completeOnEndPlay && apexSubsystem != nullptr && apexSubsystem->IsSessionInProgress())
	{
		apexSubsystem->CompleteSession(m_resultAtEndPlay, FString());
	}

	Super::EndPlay(endPlayReason);
}

void UApexSessionComponent::StartSession()
{
	UApexSubsystem* apexSubsystem = UApexSubsystem::Get(this);
	if (apexSubsystem == nullptr)
	{
		OnSessionFailed.Broadcast(
			FApexResponse::MakeFailure(EApexErrorCode::INVALID_STATE, TEXT("No Apex subsystem in this world.")));
		return;
	}

	if (apexSubsystem->IsLoggedIn() || !m_loginWithLaunchToken)
	{
		Join();
		return;
	}

	TWeakObjectPtr<UApexSessionComponent> weakThis(this);
	apexSubsystem->LoginWithToken(FString(), [weakThis](const FApexResponse& response, const FApexUser&)
	{
		if (!weakThis.IsValid())
		{
			return;
		}

		if (response.IsSuccess())
		{
			weakThis->Join();
		}
		else
		{
			weakThis->OnSessionFailed.Broadcast(response);
		}
	});
}

void UApexSessionComponent::Join()
{
	UApexSubsystem* apexSubsystem = UApexSubsystem::Get(this);
	if (apexSubsystem == nullptr)
	{
		return;
	}

	TWeakObjectPtr<UApexSessionComponent> weakThis(this);
	apexSubsystem->JoinSession(m_scenarioId, m_sessionDataJson,
							   [weakThis](const FApexResponse& response, int64 sessionId)
	{
		if (!weakThis.IsValid())
		{
			return;
		}

		if (response.IsSuccess())
		{
			weakThis->OnSessionJoined.Broadcast(sessionId);
		}
		else
		{
			weakThis->OnSessionFailed.Broadcast(response);
		}
	});
}

void UApexSessionComponent::CompleteSession(const FApexSessionResult& sessionResult, const FString& resultDataJson)
{
	UApexSubsystem* apexSubsystem = UApexSubsystem::Get(this);
	if (apexSubsystem == nullptr)
	{
		return;
	}

	TWeakObjectPtr<UApexSessionComponent> weakThis(this);
	apexSubsystem->CompleteSession(sessionResult, resultDataJson, [weakThis](const FApexResponse& response)
	{
		if (weakThis.IsValid() && !response.IsSuccess())
		{
			weakThis->OnSessionFailed.Broadcast(response);
		}
	});
}
