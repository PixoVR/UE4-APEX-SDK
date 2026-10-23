// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#include "ApexAsyncActions.h"

#include "ApexSubsystem.h"

void UApexAsyncActionBase::Prepare(UObject* worldContextObject)
{
	m_worldContextObject = worldContextObject;
	RegisterWithGameInstance(worldContextObject);
}

UApexSubsystem* UApexAsyncActionBase::GetSubsystemOrFail()
{
	UApexSubsystem* subsystem = UApexSubsystem::Get(m_worldContextObject.Get());
	if (subsystem == nullptr)
	{
		ReportMissingSubsystem(
			FApexResponse::MakeFailure(EApexErrorCode::INVALID_STATE, TEXT("No Apex subsystem in this world.")));
		SetReadyToDestroy();
	}

	return subsystem;
}

// ---------------------------------------------------------------- Login

UApexLoginAsyncAction* UApexLoginAsyncAction::Login(UObject* worldContextObject, const FString& username,
													const FString& password)
{
	UApexLoginAsyncAction* action = NewObject<UApexLoginAsyncAction>();
	action->m_loginKind = EApexLoginKind::PASSWORD;
	action->m_firstText = username;
	action->m_secondText = password;
	action->Prepare(worldContextObject);
	return action;
}

UApexLoginAsyncAction* UApexLoginAsyncAction::LoginWithToken(UObject* worldContextObject, const FString& userToken)
{
	UApexLoginAsyncAction* action = NewObject<UApexLoginAsyncAction>();
	action->m_loginKind = EApexLoginKind::TOKEN;
	action->m_firstText = userToken;
	action->Prepare(worldContextObject);
	return action;
}

UApexLoginAsyncAction* UApexLoginAsyncAction::QuickIdLogin(UObject* worldContextObject, const FString& username,
														   const FString& serialNumber)
{
	UApexLoginAsyncAction* action = NewObject<UApexLoginAsyncAction>();
	action->m_loginKind = EApexLoginKind::QUICK_ID;
	action->m_firstText = username;
	action->m_secondText = serialNumber;
	action->Prepare(worldContextObject);
	return action;
}

void UApexLoginAsyncAction::Activate()
{
	UApexSubsystem* subsystem = GetSubsystemOrFail();
	if (subsystem == nullptr)
	{
		return;
	}

	TWeakObjectPtr<UApexLoginAsyncAction> weakThis(this);
	auto onResult = [weakThis](const FApexResponse& response, const FApexUser& user)
	{
		if (weakThis.IsValid())
		{
			weakThis->Finish(response, user);
		}
	};
	switch (m_loginKind)
	{
		case EApexLoginKind::TOKEN:
			subsystem->LoginWithToken(m_firstText, onResult);
			break;
		case EApexLoginKind::QUICK_ID:
			subsystem->QuickIdLogin(m_secondText, m_firstText, onResult);
			break;
		default:
			subsystem->Login(m_firstText, m_secondText, onResult);
			break;
	}
}

void UApexLoginAsyncAction::ReportMissingSubsystem(const FApexResponse& response)
{
	OnFailure.Broadcast(response, FApexUser());
}

void UApexLoginAsyncAction::Finish(const FApexResponse& response, const FApexUser& user)
{
	if (response.IsSuccess())
	{
		OnSuccess.Broadcast(response, user);
	}
	else
	{
		OnFailure.Broadcast(response, user);
	}

	SetReadyToDestroy();
}

UApexAuthorizationCodeAsyncAction* UApexAuthorizationCodeAsyncAction::RequestAuthorizationCode(
	UObject* worldContextObject)
{
	UApexAuthorizationCodeAsyncAction* action = NewObject<UApexAuthorizationCodeAsyncAction>();
	action->Prepare(worldContextObject);
	return action;
}

void UApexAuthorizationCodeAsyncAction::Activate()
{
	UApexSubsystem* subsystem = GetSubsystemOrFail();
	if (subsystem == nullptr)
	{
		return;
	}

	TWeakObjectPtr<UApexAuthorizationCodeAsyncAction> weakThis(this);
	subsystem->RequestAuthorizationCode(
		[weakThis](const FString& authorizationCode)
	{
		if (weakThis.IsValid())
		{
			weakThis->OnCodeReceived.Broadcast(authorizationCode);
		}
	}, [weakThis](const FApexResponse& response, const FApexUser& user)
	{
		if (!weakThis.IsValid())
		{
			return;
		}

		if (response.IsSuccess())
		{
			weakThis->OnSuccess.Broadcast(response, user);
		}
		else
		{
			weakThis->OnFailure.Broadcast(response, user);
		}

		weakThis->SetReadyToDestroy();
	});
}

void UApexAuthorizationCodeAsyncAction::ReportMissingSubsystem(const FApexResponse& response)
{
	OnFailure.Broadcast(response, FApexUser());
}

// ---------------------------------------------------------------- Plain requests

UApexRequestAsyncAction* UApexRequestAsyncAction::Ping(UObject* worldContextObject)
{
	UApexRequestAsyncAction* action = NewObject<UApexRequestAsyncAction>();
	action->m_startCall = [](UApexSubsystem& subsystem, TFunction<void(const FApexResponse&)> onResult)
	{
		subsystem.Ping(MoveTemp(onResult));
	};
	action->Prepare(worldContextObject);
	return action;
}

UApexRequestAsyncAction* UApexRequestAsyncAction::SendSessionEvent(UObject* worldContextObject, const FString& action,
																   const FString& targetObject,
																   const FString& eventDataJson)
{
	UApexRequestAsyncAction* asyncAction = NewObject<UApexRequestAsyncAction>();
	asyncAction->m_startCall =
		[action, targetObject, eventDataJson](UApexSubsystem& subsystem, TFunction<void(const FApexResponse&)> onResult)
	{
		subsystem.SendSessionEvent(action, targetObject, eventDataJson, MoveTemp(onResult));
	};
	asyncAction->Prepare(worldContextObject);
	return asyncAction;
}

UApexRequestAsyncAction* UApexRequestAsyncAction::SendSessionStatement(UObject* worldContextObject,
																	   const FString& statementJson)
{
	UApexRequestAsyncAction* action = NewObject<UApexRequestAsyncAction>();
	action->m_startCall = [statementJson](UApexSubsystem& subsystem, TFunction<void(const FApexResponse&)> onResult)
	{
		subsystem.SendSessionStatement(statementJson, MoveTemp(onResult));
	};
	action->Prepare(worldContextObject);
	return action;
}

UApexRequestAsyncAction* UApexRequestAsyncAction::CompleteSession(UObject* worldContextObject,
																  const FApexSessionResult& sessionResult,
																  const FString& resultDataJson)
{
	UApexRequestAsyncAction* action = NewObject<UApexRequestAsyncAction>();
	action->m_startCall =
		[sessionResult, resultDataJson](UApexSubsystem& subsystem, TFunction<void(const FApexResponse&)> onResult)
	{
		subsystem.CompleteSession(sessionResult, resultDataJson, MoveTemp(onResult));
	};
	action->Prepare(worldContextObject);
	return action;
}

void UApexRequestAsyncAction::Activate()
{
	UApexSubsystem* subsystem = GetSubsystemOrFail();
	if (subsystem == nullptr || !m_startCall)
	{
		return;
	}

	TWeakObjectPtr<UApexRequestAsyncAction> weakThis(this);
	m_startCall(*subsystem, [weakThis](const FApexResponse& response)
	{
		if (!weakThis.IsValid())
		{
			return;
		}

		if (response.IsSuccess())
		{
			weakThis->OnSuccess.Broadcast(response);
		}
		else
		{
			weakThis->OnFailure.Broadcast(response);
		}

		weakThis->SetReadyToDestroy();
	});
}

void UApexRequestAsyncAction::ReportMissingSubsystem(const FApexResponse& response)
{
	OnFailure.Broadcast(response);
}

// ---------------------------------------------------------------- Calls with a typed result

// Each of these finishes the same way: Success or Failure with the response and the parsed result.
namespace
{
// Returns the lambda itself: TFunction will not wrap a TFunction of another signature.
template <typename TAsyncAction, typename TResult>
auto MakeFinishCallback(TAsyncAction* asyncAction)
{
	return
		[weakAction = TWeakObjectPtr<TAsyncAction>(asyncAction)](const FApexResponse& response, const TResult& result)
	{
		if (!weakAction.IsValid())
		{
			return;
		}

		if (response.IsSuccess())
		{
			weakAction->OnSuccess.Broadcast(response, result);
		}
		else
		{
			weakAction->OnFailure.Broadcast(response, result);
		}

		weakAction->SetReadyToDestroy();
	};
}
}  // namespace

UApexJoinSessionAsyncAction* UApexJoinSessionAsyncAction::JoinSession(UObject* worldContextObject,
																	  const FString& scenarioId,
																	  const FString& sessionDataJson)
{
	UApexJoinSessionAsyncAction* action = NewObject<UApexJoinSessionAsyncAction>();
	action->m_scenarioId = scenarioId;
	action->m_sessionDataJson = sessionDataJson;
	action->Prepare(worldContextObject);
	return action;
}

void UApexJoinSessionAsyncAction::Activate()
{
	if (UApexSubsystem* subsystem = GetSubsystemOrFail())
	{
		subsystem->JoinSession(m_scenarioId, m_sessionDataJson,
							   MakeFinishCallback<UApexJoinSessionAsyncAction, int64>(this));
	}
}

void UApexJoinSessionAsyncAction::ReportMissingSubsystem(const FApexResponse& response)
{
	OnFailure.Broadcast(response, 0);
}

UApexModuleAccessAsyncAction* UApexModuleAccessAsyncAction::CheckModuleAccess(UObject* worldContextObject,
																			  int32 moduleId)
{
	UApexModuleAccessAsyncAction* action = NewObject<UApexModuleAccessAsyncAction>();
	action->m_moduleId = moduleId;
	action->Prepare(worldContextObject);
	return action;
}

void UApexModuleAccessAsyncAction::Activate()
{
	if (UApexSubsystem* subsystem = GetSubsystemOrFail())
	{
		subsystem->CheckModuleAccess(m_moduleId,
									 MakeFinishCallback<UApexModuleAccessAsyncAction, FApexModuleAccess>(this));
	}
}

void UApexModuleAccessAsyncAction::ReportMissingSubsystem(const FApexResponse& response)
{
	OnFailure.Broadcast(response, FApexModuleAccess());
}

UApexModuleListAsyncAction* UApexModuleListAsyncAction::GetModuleList(UObject* worldContextObject,
																	  const FString& platformName)
{
	UApexModuleListAsyncAction* action = NewObject<UApexModuleListAsyncAction>();
	action->m_platformName = platformName;
	action->Prepare(worldContextObject);
	return action;
}

void UApexModuleListAsyncAction::Activate()
{
	if (UApexSubsystem* subsystem = GetSubsystemOrFail())
	{
		subsystem->GetModuleList(m_platformName,
								 MakeFinishCallback<UApexModuleListAsyncAction, TArray<FApexModule>>(this));
	}
}

void UApexModuleListAsyncAction::ReportMissingSubsystem(const FApexResponse& response)
{
	OnFailure.Broadcast(response, TArray<FApexModule>());
}

UApexModulePageAsyncAction* UApexModulePageAsyncAction::GetModulePage(UObject* worldContextObject,
																	  const FString& platformName, int32 lifecycles,
																	  EApexModuleSortOrder sortOrder, int32 pageIndex,
																	  int32 pageSize, bool refresh)
{
	UApexModulePageAsyncAction* action = NewObject<UApexModulePageAsyncAction>();
	action->m_platformName = platformName;
	action->m_lifecycles = lifecycles;
	action->m_sortOrder = sortOrder;
	action->m_pageIndex = pageIndex;
	action->m_pageSize = pageSize;
	action->m_refresh = refresh;
	action->Prepare(worldContextObject);
	return action;
}

void UApexModulePageAsyncAction::Activate()
{
	if (UApexSubsystem* subsystem = GetSubsystemOrFail())
	{
		subsystem->GetModulePage(m_platformName, static_cast<EApexModuleLifecycle>(m_lifecycles), m_sortOrder,
								 m_pageIndex, m_pageSize, m_refresh,
								 MakeFinishCallback<UApexModulePageAsyncAction, FApexModulePage>(this));
	}
}

void UApexModulePageAsyncAction::ReportMissingSubsystem(const FApexResponse& response)
{
	OnFailure.Broadcast(response, FApexModulePage());
}

UApexUserModulesAsyncAction* UApexUserModulesAsyncAction::GetUserModules(UObject* worldContextObject, int32 userId)
{
	UApexUserModulesAsyncAction* action = NewObject<UApexUserModulesAsyncAction>();
	action->m_userId = userId;
	action->Prepare(worldContextObject);
	return action;
}

void UApexUserModulesAsyncAction::Activate()
{
	if (UApexSubsystem* subsystem = GetSubsystemOrFail())
	{
		subsystem->GetUserModules(m_userId,
								  MakeFinishCallback<UApexUserModulesAsyncAction, TArray<FApexUserModules>>(this));
	}
}

void UApexUserModulesAsyncAction::ReportMissingSubsystem(const FApexResponse& response)
{
	OnFailure.Broadcast(response, TArray<FApexUserModules>());
}

UApexQuickIdUsersAsyncAction* UApexQuickIdUsersAsyncAction::GetQuickIdUsers(UObject* worldContextObject,
																			const FString& serialNumber)
{
	UApexQuickIdUsersAsyncAction* action = NewObject<UApexQuickIdUsersAsyncAction>();
	action->m_serialNumber = serialNumber;
	action->Prepare(worldContextObject);
	return action;
}

void UApexQuickIdUsersAsyncAction::Activate()
{
	if (UApexSubsystem* subsystem = GetSubsystemOrFail())
	{
		subsystem->QuickIdGetUsers(m_serialNumber,
								   MakeFinishCallback<UApexQuickIdUsersAsyncAction, FApexQuickIdUsers>(this));
	}
}

void UApexQuickIdUsersAsyncAction::ReportMissingSubsystem(const FApexResponse& response)
{
	OnFailure.Broadcast(response, FApexQuickIdUsers());
}

UApexLoginCodeAsyncAction* UApexLoginCodeAsyncAction::GenerateLoginCode(UObject* worldContextObject, int32 userId)
{
	UApexLoginCodeAsyncAction* action = NewObject<UApexLoginCodeAsyncAction>();
	action->m_userId = userId;
	action->Prepare(worldContextObject);
	return action;
}

void UApexLoginCodeAsyncAction::Activate()
{
	if (UApexSubsystem* subsystem = GetSubsystemOrFail())
	{
		subsystem->GenerateLoginCode(m_userId, MakeFinishCallback<UApexLoginCodeAsyncAction, FApexLoginCode>(this));
	}
}

void UApexLoginCodeAsyncAction::ReportMissingSubsystem(const FApexResponse& response)
{
	OnFailure.Broadcast(response, FApexLoginCode());
}
