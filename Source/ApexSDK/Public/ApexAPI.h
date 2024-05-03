// Copyright 2022 PixoVR Corp. All Rights Reserved.

// This cannot be #pragma once'd.
#undef PACKAGE_SCOPE
#ifdef APEXSDK_PACKAGE
#define PACKAGE_SCOPE public
#else
#define PACKAGE_SCOPE protected
#endif

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Misc/DateTime.h"
#include "ApexSDKDefines.h"
#include "XAPITypes.h"
#include "ApexTypes.h"
#include "ApexWebSocket.h"
#include "ApexAPI.generated.h"

UENUM(BlueprintType, Category = "APEX Responses", meta = (ScriptName = "MultiUserConnectionErrorType"))
enum EApexRequestType
{
	Ping,
	Login,
	Logout,
	JoinSession,
	SessionEvent,
	CompleteSession
};

#pragma region EventDelegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnApexWebSocketConnected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnApexWebSocketConnectFailed, const FString&, Error);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnApexWebSocketDisconnected, int32, StatusCode, const FString&, Reason, bool, bWasClean);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnApexAuthCodeRetrieved, const FString&, AuthorizationCode);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnApexRequestComplete, EApexRequestType, RequestType, const class UVaRestRequestJSON*, Request);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnApexRequestFail, EApexRequestType, RequestType, const class UVaRestRequestJSON*, Request, FAPEXRequestFailed, FailedRequestResponse);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnApexTopTenDataComplete, const class UVaRestRequestJSON*, Request, FTopTenData, TopTenData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnApexTopTenDataFail, const class UVaRestRequestJSON*, Request, FAPEXRequestFailed, FailedFetchResponse);

DECLARE_MULTICAST_DELEGATE(FOnStaticApexWebSocketConnected);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnStaticApexWebSocketConnectFailed, const FString&);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnStaticApexWebSocketDisconnected, int32, const FString&, bool);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnStaticApexAuthCodeRetrieved, const FString&);

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnStaticApexRequestComplete, EApexRequestType, const class UVaRestRequestJSON*);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnStaticApexRequestFail, EApexRequestType, const class UVaRestRequestJSON*);

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnStaticApexTopTenDataComplete, const class UVaRestRequestJSON*, FTopTenData);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnStaticApexTopTenDataFail, const class UVaRestRequestJSON*, FAPEXRequestFailed);
#pragma endregion EventDelegates

/**
	\class UApexAPI
	\brief The main class to access the Apex platform API.

	UApexAPI contains functionality to reach the Apex API. Information about the module, 
	platform and HMD device is readable from here as well. It also contains information
	about the current user.

	\todo Uncomment out the Logout API call once it's implemented.
 */
UCLASS(BlueprintType, Blueprintable)
class APEXSDK_API UApexAPI : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

#pragma region PublicProperties

	/**
	* \brief The API URL
	*
	* \sa URL
	*/
	UPROPERTY(BlueprintReadOnly)
	FString URL;

	/**
	* \brief The API Websocket URL
	*
	* \sa WebSocketURL
	*/
	UPROPERTY(BlueprintReadOnly)
	FString WebSocketURL;

	UPROPERTY(BlueprintReadOnly)
	FString ScenarioId;

	UPROPERTY(BlueprintReadWrite, Meta=(DisplayName="Module Id"))
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
#pragma endregion PublicProperties

PACKAGE_SCOPE:

	UPROPERTY(BlueprintReadOnly)
	bool bSessionInProgress;

	UPROPERTY(BlueprintReadOnly)
	bool bWebSocketEnabled;

public:
	UApexAPI();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category = "Apex|API")
	void Ping();

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Apex|API")
	bool HasLaunchAuthToken();

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Apex|API")
	bool IsLoggedIn();

	UFUNCTION(BlueprintCallable, Category = "Apex|API")
	void LoginWithAuthToken();

	UFUNCTION(BlueprintCallable, Category = "Apex|API")
	void Login(const FString& InUserName, const FString& InPassword, bool bShouldLogout = true);

	UFUNCTION(BlueprintCallable, Category = "Apex|API")
	void Logout();

	UFUNCTION(BlueprintCallable, Category = "Apex|API")
	bool IsWebSocketConnected();

	UFUNCTION(BlueprintCallable, Category = "Apex|API", meta = (Tooltip = "This function only needs to be called when a user has logged out."))
	void ConnectWebSocketService();

	UFUNCTION(BlueprintCallable, Category = "Apex|API")
	void RequestAuthorizationCode();

	UFUNCTION(BlueprintCallable, Category = "Apex|API")
	bool JoinSession(FString InScenarioId, const FXAPIExtension& InContextExtension, bool bIsMultiplayer = false);

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

	UFUNCTION(BlueprintCallable, Category = "Apex|API")
	void FilloutLaunchToken();

	UFUNCTION(BlueprintCallable, Category = "Apex|API")
	void VerifyModuleAccess(int InModuleId);

protected:

#pragma region PrivateFunctions
	TSharedRef<FJsonObject> GetUserJsonObject(TSharedRef<FJsonObject>& RootJsonObject);
	bool FetchTopTen(int UserId, float TimePriorToToday, FString Scenario, int OrgUnit);
	void HandleLogin(class UVaRestJsonObject* LoginJsonObject);
	void SetupModuleSettings();
	void SendHeartbeat();

	void OnLoginComplete(const class UVaRestRequestJSON* Request);
	void OnLoginFail(const class UVaRestRequestJSON* Request);
	void OnLogoutComplete(const class UVaRestRequestJSON* Request);
	void OnLogoutFail(const class UVaRestRequestJSON* Request);
	void OnUserVerificationComplete(const class UVaRestRequestJSON* Request);
	void OnUserVerificationFail(const class UVaRestRequestJSON* Request);
	void OnPingComplete(const class UVaRestRequestJSON* Request);
	void OnPingFail(const class UVaRestRequestJSON* Request);
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
	class UApexSDKSettings* ApexSettings;
	class UPixoLauncherSubsystem* LauncherSubsystem;

	FString LaunchToken;
	FTimerHandle HeartbeatHandle;
	int SessionId;

	TUniquePtr<ApexWebSocket> ApexSocket;
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

	// Web Socket Events
	UPROPERTY(BlueprintAssignable, Category = "Apex|Event")
	FOnApexWebSocketConnected OnWebSocketConnected;
	
	UPROPERTY(BlueprintAssignable, Category = "Apex|Event")
	FOnApexWebSocketConnectFailed OnWebSocketConnectFailed;
	
	UPROPERTY(BlueprintAssignable, Category = "Apex|Event")
	FOnApexWebSocketDisconnected OnWebSocketDisconnected;

	UPROPERTY(BlueprintAssignable, Category = "Apex|Event")
	FOnApexAuthCodeRetrieved OnAuthorizationCodeRetrieved;

	FOnStaticApexWebSocketConnected OnStaticWebSocketConnected;

	FOnStaticApexWebSocketConnectFailed OnStaticWebSocketConnectFailed;

	FOnStaticApexWebSocketDisconnected OnStaticWebSocketDisconnected;

	FOnStaticApexAuthCodeRetrieved OnStaticAuthorizationCodeRetrieved;
#pragma endregion Events
};
