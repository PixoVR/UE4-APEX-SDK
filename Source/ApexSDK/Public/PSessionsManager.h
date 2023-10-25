// Copyright 2022 PixoVR Corp. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "Misc/DateTime.h"
#include "ApexSDKDefines.h"
#include "XAPITypes.h"
#include "ApexTypes.h"
#include "ApexWebSocket.h"
#include "PSessionsManager.generated.h"

UENUM(BlueprintType, Category = "APEX Responses", meta = (ScriptName = "MultiUserConnectionErrorType"))
enum EApexRequestType
{
	JoinSession,
	SessionEvent,
	CompleteSession
};

#pragma region EventDelegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnApexRequestComplete, EApexRequestType, RequestType, const class UVaRestRequestJSON*, Request);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnApexRequestFail, EApexRequestType, RequestType, FAPEXRequestFailed, FailedRequestResponse, const class UVaRestRequestJSON*, Request);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnApexTopTenDataComplete, FTopTenData, TopTenData, const class UVaRestRequestJSON*, Request);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnApexTopTenDataFail, FAPEXRequestFailed, FailedFetchResponse, const class UVaRestRequestJSON*, Request);

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnStaticApexRequestComplete, EApexRequestType, const class UVaRestRequestJSON*);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnStaticApexRequestFail, EApexRequestType, FAPEXRequestFailed, const class UVaRestRequestJSON*);

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnStaticApexTopTenDataComplete, FTopTenData, const class UVaRestRequestJSON*);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnStaticApexTopTenDataFail, FAPEXRequestFailed, const class UVaRestRequestJSON*);
#pragma endregion EventDelegates

/**
	\class UApexAPI
	\brief The main class to access the Apex platform API.

	UApexAPI contains functionality to reach the Apex API. Information about the module, 
	platform and HMD device is readable from here as well. It also contains information
	about the current user.

	\todo Uncomment out the Logout API call once it's implemented.
 */
UCLASS(BlueprintType, Blueprintable, DisplayName = "PixoVR Session Manager")
class APEXSDK_API UPSessionsManager : public UEngineSubsystem
{
	GENERATED_BODY()

public:

#pragma region PublicProperties

	UPROPERTY(BlueprintReadOnly)
	FString ScenarioId;

	UPROPERTY(BlueprintReadOnly)
	FString Platform;

	UPROPERTY(BlueprintReadOnly)
	FString DeviceId;

	UPROPERTY(BlueprintReadOnly)
	FString DeviceModel;

	UPROPERTY(BlueprintReadOnly)
	FString ClientIP;

	UPROPERTY(BlueprintReadOnly)
	FGuid CurrentSessionGuid;

#pragma endregion PublicProperties

protected:

	UPROPERTY(BlueprintReadOnly)
	bool SessionInProgress;

public:
	UPSessionsManager();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category = "Apex|API")
	bool JoinSession(FString InScenarioId, const FXAPIExtension& InContextExtension);

	UFUNCTION(BlueprintCallable, Category = "Apex|API")
	bool CompleteSession(const FSessionData& InSessionData);

	UFUNCTION(BlueprintCallable, Category = "Apex|API")
	bool SendSessionEvent(const FXAPIStatement& InStatement);

	UFUNCTION(BlueprintCallable, Category = "Apex|API")
	bool FetchTopTen(FString Scenario, int OrgUnit);

	UFUNCTION(BlueprintCallable, Category = "Apex|API", meta = (DisplayName = "Fetch Top Ten By User"))
	bool FetchTopTenByCurrentUser(FString Scenario);

	UFUNCTION(BlueprintCallable, Category = "Apex|API", meta=(DisplayName = "Fetch Top Ten By User"))
	bool FetchTopTenByUser(int UserId, FString Scenario, int OrgUnit);

	UFUNCTION(BlueprintCallable, Category = "Apex|API")
	bool FetchTopTenByTimeRange(float TimePriorToToday, FString Scenario, int OrgUnit);

protected:

#pragma region PrivateFunctions
	void OnAuthComplete(const FUserSessionData& AuthorizedUser, const class UVaRestRequestJSON* Request);
	void OnLogoutComplete();

	bool FetchTopTen(int UserId, float TimePriorToToday, FString Scenario, int OrgUnit);

	void OnJoinSessionComplete(const class UVaRestRequestJSON* Request);
	void OnJoinSessionFail(const class UVaRestRequestJSON* Request);
	void OnCompleteSessionComplete(const class UVaRestRequestJSON* Request);
	void OnCompleteSessionFail(const class UVaRestRequestJSON* Request);
	void OnSendSessionEventComplete(const class UVaRestRequestJSON* Request);
	void OnSendSessionEventFail(const class UVaRestRequestJSON* Request);
	void OnTopTenComplete(const class UVaRestRequestJSON* Request);
	void OnTopTenFail(const class UVaRestRequestJSON* Request);
#pragma endregion PrivateFunctions

private:

#pragma region PrivateVariables
	class UVaRestSubsystem* VaRestSubsystem;
	class UPPlatformAuth* AuthorizationSubsystem;

	FString URL;
	int ModuleId;
	FString ModuleVersion;
	FUserSessionData CurrentUser;
#pragma endregion PrivateVariables

public:

#pragma region Events
	// General Apex Requests
	UPROPERTY(BlueprintAssignable, Category = "Apex|Event")
	FOnApexRequestComplete OnRequestComplete;

	UPROPERTY(BlueprintAssignable, Category = "Apex|Event")
	FOnApexRequestFail OnRequestFail;

	FOnStaticApexRequestComplete OnStaticRequestComplete;
	FOnStaticApexRequestFail OnStaticRequestFail;

	// Fetch Score Requests
	UPROPERTY(BlueprintAssignable, Category = "Apex|Event")
	FOnApexTopTenDataComplete OnFetchTopTenComplete;
	
	UPROPERTY(BlueprintAssignable, Category = "Apex|Event")
	FOnApexTopTenDataFail OnFetchTopTenFail;

	FOnStaticApexTopTenDataComplete OnStaticFetchTopTenComplete;

	FOnStaticApexTopTenDataFail OnStaticFetchTopTenFail;
#pragma endregion Events
};
