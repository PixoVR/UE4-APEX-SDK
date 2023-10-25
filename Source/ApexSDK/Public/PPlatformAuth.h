// Copyright 2022 PixoVR Corp. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "Misc/DateTime.h"
#include "ApexSDKDefines.h"
#include "XAPITypes.h"
#include "ApexTypes.h"
#include "ApexWebSocket.h"
#include "PPlatformAuth.generated.h"

#pragma region EventDelegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnApexWebSocketConnected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnApexWebSocketConnectFailed, const FString&, Error);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnApexWebSocketDisconnected, int32, StatusCode, const FString&, Reason, bool, bWasClean);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnApexAuthCodeRetrieved, const FString&, AuthorizationCode);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAuthCompleted, const FUserSessionData&, AuthorizedUser, const class UVaRestRequestJSON*, Request);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAuthFailed, FAPEXRequestFailed, FailedRequestResponse, const class UVaRestRequestJSON*, Request);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLogoutCompleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLogoutFailed, FAPEXRequestFailed, FailedRequestResponse);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPingCompleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPingFailed, FAPEXRequestFailed, FailedRequestResponse);

DECLARE_MULTICAST_DELEGATE(FOnStaticApexWebSocketConnected);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnStaticApexWebSocketConnectFailed, const FString&);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnStaticApexWebSocketDisconnected, int32, const FString&, bool);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnStaticApexAuthCodeRetrieved, const FString&);

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnStaticAuthCompleted, const FUserSessionData&, const class UVaRestRequestJSON*);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnStaticAuthFailed, FAPEXRequestFailed, const class UVaRestRequestJSON*);

DECLARE_MULTICAST_DELEGATE(FOnStaticLogoutCompleted);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnStaticLogoutFailed, FAPEXRequestFailed);

DECLARE_MULTICAST_DELEGATE(FOnStaticPingCompleted);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnStaticPingFailed, FAPEXRequestFailed);

#pragma endregion EventDelegates

/**
	\class UPPlatformAuth
	\brief The subsystem to authenticate the user against the PixoVR Platform.

	UPPlatformAuth contains information about the current user.
 */
UCLASS(BlueprintType, Blueprintable, DisplayName = "PixoVR Platform Auth")
class APEXSDK_API UPPlatformAuth : public UEngineSubsystem
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

	UPROPERTY(BlueprintReadOnly, Meta=(DisplayName="Module Id"))
	int ModuleId;

	UPROPERTY(BlueprintReadOnly)
	FString ModuleVersion;

	UPROPERTY(BlueprintReadOnly)
	FUserSessionData CurrentActiveLogin;
#pragma endregion PublicProperties

protected:

	UPROPERTY(BlueprintReadOnly)
	bool WebSocketEnabled;

public:
	UPPlatformAuth();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category = "PixoVR|Platform|Event")
	void Ping();

	UFUNCTION(BlueprintPure, Category = "PixoVR|Platform|Event")
	bool IsLoggedIn();

	UFUNCTION(BlueprintCallable, Category = "PixoVR|Platform|Event")
	void LoginUser(const FString& InUsername, const FString& InPassword = TEXT(""), bool bShouldLogout = true);

	UFUNCTION(BlueprintCallable, Category = "PixoVR|Platform|Event")
	void Login(const FApexLoginInfo& InLoginInfo, bool bShouldLogout = true);

	UFUNCTION(BlueprintCallable, Category = "PixoVR|Platform|Event")
	void Logout();

	UFUNCTION(BlueprintCallable, Category = "PixoVR|Platform|Event")
	bool IsWebSocketConnected();

	UFUNCTION(BlueprintCallable, Category = "PixoVR|Platform|Event", meta = (Tooltip = "This function only needs to be called when a user has logged out."))
	void ConnectWebSocketService();

	UFUNCTION(BlueprintCallable, Category = "PixoVR|Platform|Event")
	void RequestAuthorizationCode();

protected:

#pragma region PrivateFunctions
	void HandleLogin(class UVaRestJsonObject* LoginJsonObject);
	void VerifyModuleAccess();
	void SetupModuleSettings();
	void UpdateSettings(class UPPlatformCoreSettings* InSettings);

	void OnLoginComplete(const class UVaRestRequestJSON* Request);
	void OnLoginFail(const class UVaRestRequestJSON* Request);
	void OnLogoutComplete(const class UVaRestRequestJSON* Request);
	void OnLogoutFail(const class UVaRestRequestJSON* Request);
	void OnUserVerificationComplete(const class UVaRestRequestJSON* Request);
	void OnUserVerificationFail(const class UVaRestRequestJSON* Request);
	void OnPingComplete(const class UVaRestRequestJSON* Request);
	void OnPingFail(const class UVaRestRequestJSON* Request);
#pragma endregion PrivateFunctions

private:

#pragma region PrivateVariables
	class UVaRestSubsystem* VaRestSubsystem;
	class UPPlatformCoreSettings* PlatformSettings;

	FString AuthorizationToken;

	TUniquePtr<ApexWebSocket> WebSocket;
#pragma endregion PrivateVariables

public:

#pragma region Events
	// General Apex Requests
	UPROPERTY(BlueprintAssignable, Category = "PixoVR|Platform|Event")
	FOnAuthCompleted OnAuthorizationCompleted;

	UPROPERTY(BlueprintAssignable, Category = "PixoVR|Platform|Event")
	FOnAuthFailed OnAuthorizationFailed;

	FOnStaticAuthCompleted OnStaticAuthorizationCompleted;
	FOnStaticAuthFailed OnStaticAuthorizationFailed;

	UPROPERTY(BlueprintAssignable, Category = "PixoVR|Platform|Event")
	FOnLogoutCompleted OnLogoutCompleted;

	UPROPERTY(BlueprintAssignable, Category = "PixoVR|Platform|Event")
	FOnLogoutFailed OnLogoutFailed;

	FOnStaticLogoutCompleted OnStaticLogoutCompleted;
	FOnStaticLogoutFailed OnStaticLogoutFailed;

	UPROPERTY(BlueprintAssignable, Category = "PixoVR|Platform|Event")
	FOnPingCompleted OnPingCompleted;

	UPROPERTY(BlueprintAssignable, Category = "PixoVR|Platform|Event")
	FOnPingFailed OnPingFailed;

	FOnStaticPingCompleted OnStaticPingCompleted;
	FOnStaticPingFailed OnStaticPingFailed;

	// Web Socket Events
	UPROPERTY(BlueprintAssignable, Category = "PixoVR|Platform|Event")
	FOnApexWebSocketConnected OnWebSocketConnected;
	
	UPROPERTY(BlueprintAssignable, Category = "PixoVR|Platform|Event")
	FOnApexWebSocketConnectFailed OnWebSocketConnectFailed;
	
	UPROPERTY(BlueprintAssignable, Category = "PixoVR|Platform|Event")
	FOnApexWebSocketDisconnected OnWebSocketDisconnected;

	UPROPERTY(BlueprintAssignable, Category = "PixoVR|Platform|Event")
	FOnApexAuthCodeRetrieved OnAuthorizationCodeRetrieved;

	FOnStaticApexWebSocketConnected OnStaticWebSocketConnected;

	FOnStaticApexWebSocketConnectFailed OnStaticWebSocketConnectFailed;

	FOnStaticApexWebSocketDisconnected OnStaticWebSocketDisconnected;

	FOnStaticApexAuthCodeRetrieved OnStaticAuthorizationCodeRetrieved;
#pragma endregion Events
};
