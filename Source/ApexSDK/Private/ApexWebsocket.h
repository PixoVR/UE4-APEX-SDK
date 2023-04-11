// Copyright 2023 PixoVR Corp. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ApexSDKDefines.h"
#include "ApexJSON.h"
#include "IWebSocket.h"
#include "ApexWebsocket.generated.h"

enum ApexWebSocketRequestType : int
{
    AuthorizationCode
};

USTRUCT(BlueprintType)
struct APEXSDK_API FAPEXAuthenticationCode : public FJsonable
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Authentication Code")
    FString AuthCode;

    FAPEXAuthenticationCode()
    {
        // Create a default
        AuthCode = "XXX-XXX";
    }

    virtual void FromJsonObject(const TSharedPtr<FJsonObject>& JObject) override
    {
        if (JObject->HasField("auth_code"))
            AuthCode = JObject->GetStringField("auth_code");
    }
};

class ApexWebSocket
{
public:

    ApexWebSocket();
    ~ApexWebSocket();

    DECLARE_EVENT_OneParam(ApexWebSocket, FApexSocketAuthCodeRetrieved, FString AuthenticationCode);
    FApexSocketAuthCodeRetrieved& OnApexAuthCodeRetrieved() { return ApexAuthCodeRetrievedEvent; }

    DECLARE_EVENT_OneParam(ApexWebSocket, FApexSocketAuthenticateComplete, class UVaRestJsonObject* LoginJsonObject);
    FApexSocketAuthenticateComplete& OnApexAuthenticateComplete() { return ApexAuthenticateCompleteEvent; }

    DECLARE_EVENT(ApexWebSocket, FApexSocketConnected);
    FApexSocketConnected& OnApexWebSocketConnected() { return ApexConnectedEvent; }

    DECLARE_EVENT_OneParam(ApexWebSocket, FApexSocketConnectFailed, const FString&);
    FApexSocketConnectFailed& OnApexWebSocketConnectFailed() { return ApexConnectFailedEvent; }

    DECLARE_EVENT_ThreeParams(ApexWebSocket, FApexSocketDisconnected, int32 StatusCode, const FString& Reason, bool bWasClean);
    FApexSocketDisconnected& OnApexWebSocketDisconnected() { return ApexDisconnectedEvent; }

    bool AttemptConnect(FString Uri, FString Protocol = "wss");
    bool IsWebsocketConnected();
    void RequestAuthorizationCode();

private:
    void SetupSocket(TSharedPtr<IWebSocket>& Socket, FString Uri, FString Protocol);
    void ProcessNextRequest();
    void CleanUpRequests();

    void SendData(FString Data);

    void HandleAuthCodeReceived(UVaRestJsonObject* JsonObject);
    void HandleAuthenticateComplete(UVaRestJsonObject* JsonObject);

private:
    TSharedPtr<IWebSocket> WebSocket;
    TArray<ApexWebSocketRequestType> PendingWebsocketRequests;

    FApexSocketAuthCodeRetrieved ApexAuthCodeRetrievedEvent;
    FApexSocketAuthenticateComplete ApexAuthenticateCompleteEvent;
    FApexSocketConnected ApexConnectedEvent;
    FApexSocketConnectFailed ApexConnectFailedEvent;
    FApexSocketDisconnected ApexDisconnectedEvent;
};