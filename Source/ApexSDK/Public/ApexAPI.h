// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ApexSDKDefines.h"
#include "XAPITypes.h"
#include "ApexTypes.h"
#include "ApexAPI.generated.h"

UENUM(BlueprintType, Category = "APEX Responses", meta = (ScriptName = "MultiUserConnectionErrorType"))
enum ApexRequestType
{
	Ping,
	Login,
	JoinSession,
	SessionEvent,
	CompleteSession
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnApexRequestComplete, ApexRequestType, RequestType, class UVaRestRequestJSON*, Request);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnApexRequestFail, ApexRequestType, RequestType, class UVaRestRequestJSON*, Request);

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnStaticApexRequestComplete, ApexRequestType, class UVaRestRequestJSON*);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnStaticApexRequestFail, ApexRequestType, class UVaRestRequestJSON*);

/**
 *
 */
UCLASS(BlueprintType, Blueprintable)
class APEXSDK_API UApexAPI : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

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
	bool JoinSession(FString InScenarioID, const FXAPIExtension& InContextExtension);

	UFUNCTION(BlueprintCallable, Category = "Apex|API")
	bool CompleteSession(const FSessionData& InSessionData);

	UFUNCTION(BlueprintCallable, Category = "Apex|API")
	bool SendSessionEvent(const FXAPIStatement& InStatement);


protected:
	class UVaRestSubsystem* VaRestSubsystem;
	class UApexSDKSettings* ApexSettings;

	FString AuthorizationToken;

	FScriptDelegate OnPingCompleteDelegate;
	FScriptDelegate OnPingFailDelegate;
	FScriptDelegate OnLoginCompleteDelegate;
	FScriptDelegate OnLoginFailDelegate;
	FScriptDelegate OnJoinSessionCompleteDelegate;
	FScriptDelegate OnJoinSessionFailDelegate;
	FScriptDelegate OnCompleteSessionCompleteDelegate;
	FScriptDelegate OnCompleteSessionFailDelegate;
	FScriptDelegate OnSendSessionEventCompleteDelegate;
	FScriptDelegate OnSendSessionEventFailDelegate;

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

public:

	UFUNCTION(BlueprintCallable, Category = "Apex|API")
	void OnLoginComplete(class UVaRestRequestJSON* Request);

	UFUNCTION(BlueprintCallable, Category = "Apex|API")
	void OnLoginFail(class UVaRestRequestJSON* Request);

	UFUNCTION(BlueprintCallable, Category = "Apex|API")
	void OnPingComplete(class UVaRestRequestJSON* Request);

	UFUNCTION(BlueprintCallable, Category = "Apex|API")
	void OnPingFail(class UVaRestRequestJSON* Request);

	UFUNCTION(BlueprintCallable, Category = "Apex|API")
	void OnJoinSessionComplete(class UVaRestRequestJSON* Request);

	UFUNCTION(BlueprintCallable, Category = "Apex|API")
	void OnJoinSessionFail(class UVaRestRequestJSON* Request);

	UFUNCTION(BlueprintCallable, Category = "Apex|API")
	void OnCompleteSessionComplete(class UVaRestRequestJSON* Request);

	UFUNCTION(BlueprintCallable, Category = "Apex|API")
	void OnCompleteSessionFail(class UVaRestRequestJSON* Request);

	UFUNCTION(BlueprintCallable, Category = "Apex|API")
	void OnSendSessionEventComplete(class UVaRestRequestJSON* Request);

	UFUNCTION(BlueprintCallable, Category = "Apex|API")
	void OnSendSessionEventFail(class UVaRestRequestJSON* Request);
};