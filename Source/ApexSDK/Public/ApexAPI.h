// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ApexSDKDefines.h"
#include "XAPITypes.h"
#include "ApexTypes.h"
#include "Misc/DateTime.h"
#include "ApexAPI.generated.h"

UENUM(BlueprintType, Category = "APEX Responses", meta = (ScriptName = "MultiUserConnectionErrorType"))
enum EApexRequestType
{
	Ping,
	Login,
	JoinSession,
	SessionEvent,
	CompleteSession
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnApexRequestComplete, EApexRequestType, RequestType, class UVaRestRequestJSON*, Request);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnApexRequestFail, EApexRequestType, RequestType, class UVaRestRequestJSON*, Request, FAPEXRequestFailed, FailedRequestResponse);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnApexTopTenDataComplete, class UVaRestRequestJSON*, Request, FTopTenData, TopTenData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnApexTopTenDataFail, class UVaRestRequestJSON*, Request, FAPEXRequestFailed, FailedFetchResponse);

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnStaticApexRequestComplete, EApexRequestType, class UVaRestRequestJSON*);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnStaticApexRequestFail, EApexRequestType, class UVaRestRequestJSON*);

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnStaticApexTopTenDataComplete, class UVaRestRequestJSON*, FTopTenData);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnStaticApexTopTenDataFail, class UVaRestRequestJSON*, FAPEXRequestFailed);


/**
 	\class UApexAPI
	\brief some stuff here about how the API root works.

	We want to talk a lot about Apex here in length that will wrap on new lines and take up a lot of space.

	\todo feed the dog.

	\warning blah blah

	\note blah blah
 */
UCLASS(BlueprintType, Blueprintable)
/**
	\qualifier BlueprintType
	\qualifier Blueprintable
*/
class APEXSDK_API UApexAPI : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	/**
	* \property blueprint URL
	* \brief The API URL
	* 
	* \sa ScenarioId
	*/
	UPROPERTY(BlueprintReadOnly)
	FString URL;

	UPROPERTY(BlueprintReadOnly)
	FString ScenarioId;

	UPROPERTY(BlueprintReadOnly, Meta=(DisplayName="Module Id"))
	int LoadedModuleId;

	UPROPERTY(BlueprintReadOnly)
	FString ModuleVersion;

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

	UPROPERTY(BlueprintReadOnly)
	FUserSessionData CurrentActiveLogin;

protected:

	UPROPERTY(BlueprintReadOnly)
	bool SessionInProgress;

public:
	UApexAPI();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category = "Apex|API")
	void Ping();

	UFUNCTION(BlueprintCallable, Category = "Apex|API")
	void Login(const FApexLoginInfo& InLoginInfo);

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

	bool FetchTopTen(int UserId, float TimePriorToToday, FString Scenario, int OrgUnit);
	void VerifyModuleAcess();

protected:
	class UVaRestSubsystem* VaRestSubsystem;
	class UApexSDKSettings* ApexSettings;

	FString AuthorizationToken;

	FScriptDelegate OnPingCompleteDelegate;
	FScriptDelegate OnPingFailDelegate;
	FScriptDelegate OnLoginCompleteDelegate;
	FScriptDelegate OnLoginFailDelegate;
	FScriptDelegate OnUserVerificationCompleteDelegate;
	FScriptDelegate OnUserVerificationFailDelegate;
	FScriptDelegate OnJoinSessionCompleteDelegate;
	FScriptDelegate OnJoinSessionFailDelegate;
	FScriptDelegate OnCompleteSessionCompleteDelegate;
	FScriptDelegate OnCompleteSessionFailDelegate;
	FScriptDelegate OnSendSessionEventCompleteDelegate;
	FScriptDelegate OnSendSessionEventFailDelegate;
	FScriptDelegate OnTopTenCompleteDelegate;
	FScriptDelegate OnTopTenFailDelegate;

public:

	/** Event occured when the request has been completed */
	UPROPERTY(BlueprintAssignable, Category = "Apex|Event")
	FOnApexRequestComplete OnRequestComplete;

	/** Event occured when the request wasn't successfull */
	UPROPERTY(BlueprintAssignable, Category = "Apex|Event")
	FOnApexRequestFail OnRequestFail;

	/** Event occured when the request has been completed */
	FOnStaticApexRequestComplete OnStaticRequestComplete;

	/** Event occured when the request wasn't successfull */
	FOnStaticApexRequestFail OnStaticRequestFail;

	UPROPERTY(BlueprintAssignable, Category = "Apex|Event")
	FOnApexTopTenDataComplete OnFetchTopTenComplete;

	UPROPERTY(BlueprintAssignable, Category = "Apex|Event")
	FOnApexTopTenDataFail OnFetchTopTenFail;

	FOnStaticApexTopTenDataComplete OnStaticFetchTopTenComplete;

	FOnStaticApexTopTenDataFail OnStaticFetchTopTenFail;

public:

	UFUNCTION(Category = "Apex|API")
	void OnLoginComplete(class UVaRestRequestJSON* Request);

	UFUNCTION(Category = "Apex|API")
	void OnLoginFail(class UVaRestRequestJSON* Request);

	UFUNCTION(Category = "Apex|API")
	void OnUserVerificationComplete(class UVaRestRequestJSON* Request);

	UFUNCTION(Category = "Apex|API")
	void OnUserVerificationFail(class UVaRestRequestJSON* Request);

	UFUNCTION(Category = "Apex|API")
	void OnPingComplete(class UVaRestRequestJSON* Request);

	UFUNCTION(Category = "Apex|API")
	void OnPingFail(class UVaRestRequestJSON* Request);

	UFUNCTION(Category = "Apex|API")
	void OnJoinSessionComplete(class UVaRestRequestJSON* Request);

	UFUNCTION(Category = "Apex|API")
	void OnJoinSessionFail(class UVaRestRequestJSON* Request);

	UFUNCTION(Category = "Apex|API")
	void OnCompleteSessionComplete(class UVaRestRequestJSON* Request);

	UFUNCTION(Category = "Apex|API")
	void OnCompleteSessionFail(class UVaRestRequestJSON* Request);

	UFUNCTION(Category = "Apex|API")
	void OnSendSessionEventComplete(class UVaRestRequestJSON* Request);

	UFUNCTION(Category = "Apex|API")
	void OnSendSessionEventFail(class UVaRestRequestJSON* Request);

	UFUNCTION(Category = "Apex|API")
	void OnTopTenComplete(class UVaRestRequestJSON* Request);

	UFUNCTION(Category = "Apex|API")
	void OnTopTenFail(class UVaRestRequestJSON* Request);
};
