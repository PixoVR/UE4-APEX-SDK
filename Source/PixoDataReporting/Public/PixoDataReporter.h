// Copyright 2022 PixoVR Corp. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "Misc/DateTime.h"

#include "ApexTypes.h"
#include "ApexSDKDefines.h"
#include "XAPITypes.h"

#include "PixoConfigJsonTypes.h"
#include "PixoAuthWebSocket.h"

#include "PixoDataReporter.generated.h"

UENUM(BlueprintType, Category = "Data Reporting", meta = (ScriptName = "MultiUserConnectionErrorType"))
enum EPixoDataReportType
{
	JoinSession,
	SessionEvent,
	CompleteSession
};

#pragma region EventDelegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPDataReporterReady);
DECLARE_MULTICAST_DELEGATE(FOnStaticPDataReporterReady);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDataReporterRequestComplete, EPixoDataReportType, RequestType, const class UVaRestRequestJSON*, Request);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnDataReporterRequestFail, EPixoDataReportType, RequestType, FPRequestFailed, FailedRequestResponse, const class UVaRestRequestJSON*, Request);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDataReporterTopTenDataComplete, FTopTenData, TopTenData, const class UVaRestRequestJSON*, Request);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDataReporterTopTenDataFail, FPRequestFailed, FailedFetchResponse, const class UVaRestRequestJSON*, Request);

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnStaticDataReporterRequestComplete, EPixoDataReportType, const class UVaRestRequestJSON*);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnStaticDataReporterRequestFail, EPixoDataReportType, FPRequestFailed, const class UVaRestRequestJSON*);

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnStaticDataReporterTopTenDataComplete, FTopTenData, const class UVaRestRequestJSON*);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnStaticDataReporterTopTenDataFail, FPRequestFailed, const class UVaRestRequestJSON*);
#pragma endregion EventDelegates

/**
	\class UPixoDataReporter
	\brief The main class to access the Apex platform API.

	UApexAPI contains functionality to reach the Apex API. Information about the module, 
	platform and HMD device is readable from here as well. It also contains information
	about the current user.

	\todo Uncomment out the Logout API call once it's implemented.
 */
UCLASS(BlueprintType, Blueprintable, DisplayName = "PixoVR Data Reporter", meta=(DisplayName="PixoVR Data Reporter", Keywords="Data Reporter Pixo"))
class PIXODATAREPORTING_API UPixoDataReporter : public UEngineSubsystem
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
	UPixoDataReporter();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "PixoVR|Data Reporter")
	bool JoinSession(FString InScenarioId, const FXAPIExtension& InContextExtension, bool bIsMultiplayer = false);

	UFUNCTION(BlueprintCallable, Category = "PixoVR|Data Reporter")
	bool CompleteSession(const FSessionData& InSessionData);

	UFUNCTION(BlueprintCallable, Category = "PixoVR|Data Reporter")
	bool SendSessionEvent(const FXAPIStatement& InStatement);

	UFUNCTION(BlueprintCallable, Category = "PixoVR|Data Reporter")
	bool FetchTopTen(FString Scenario, int OrgUnit);

	UFUNCTION(BlueprintCallable, Category = "PixoVR|Data Reporter", meta=(DisplayName = "Fetch Top Ten By User", Tooltip="Fetches the top ten by user. A user id or org unit less than 1 will default to the current user."))
	bool FetchTopTenByUser(FString Scenario, int UserId = -1, int OrgUnit = -1);

	UFUNCTION(BlueprintCallable, Category = "PixoVR|Data Reporter")
	bool FetchTopTenByTimeRange(float TimePriorToToday, FString Scenario, int OrgUnit);

protected:

#pragma region PrivateFunctions
	void OnAuthComplete(const FUserSessionData& AuthorizedUser, const class UVaRestRequestJSON* Request);
	void OnLogoutComplete();
	void SetupSessionManagerSettings(class UPixoConfigSettings* PlatformSettings);

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
	class UPixoAuthorizer* AuthorizationSubsystem;
	class UPixoConfigSettings* PlatformSettings;

	FString URL;
	int ModuleId;
	FString ModuleVersion;
	FUserSessionData CurrentUser;
#pragma endregion PrivateVariables

public:

#pragma region Events
	UPROPERTY(BlueprintAssignable, Category = "PixoVR|Data Reporter|Events")
	FOnPDataReporterReady OnDataReporterReady;

	FOnStaticPDataReporterReady OnStaticDataReporterReady;


	// General Apex Requests
	UPROPERTY(BlueprintAssignable, Category = "PixoVR|Data Reporter|Events")
	FOnDataReporterRequestComplete OnRequestComplete;

	UPROPERTY(BlueprintAssignable, Category = "PixoVR|Data Reporter|Events")
	FOnDataReporterRequestFail OnRequestFail;

	FOnStaticDataReporterRequestComplete OnStaticRequestComplete;
	FOnStaticDataReporterRequestFail OnStaticRequestFail;

	// Fetch Score Requests
	UPROPERTY(BlueprintAssignable, Category = "PixoVR|Data Reporter|Events")
	FOnDataReporterTopTenDataComplete OnFetchTopTenComplete;
	
	UPROPERTY(BlueprintAssignable, Category = "PixoVR|Data Reporter|Events")
	FOnDataReporterTopTenDataFail OnFetchTopTenFail;

	FOnStaticDataReporterTopTenDataComplete OnStaticFetchTopTenComplete;

	FOnStaticDataReporterTopTenDataFail OnStaticFetchTopTenFail;
#pragma endregion Events
};
