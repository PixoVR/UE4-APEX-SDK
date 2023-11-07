// Copyright 2023 PixoVR Corp. All Rights Reserved.

#include "ApexWebSocket.h"
#include "WebSocketsModule.h"
#include "IWebSocket.h"
#include "ApexLibrary.h"

DEFINE_LOG_CATEGORY_STATIC(LogApexWebSocket, Log, All);

#define LogAPEX(pmt, ...) UE_LOG(LogApexWebSocket, Log, TEXT(pmt), ##__VA_ARGS__)
#define WarnAPEX(pmt, ...) UE_LOG(LogApexWebSocket, Warning, TEXT(pmt), ##__VA_ARGS__)
#define ErrorAPEX(pmt, ...) UE_LOG(LogApexWebSocket, Error, TEXT(pmt), ##__VA_ARGS__)
#define FatalAPEX(pmt, ...) UE_LOG(LogApexWebSocket, Fatal, TEXT(pmt), ##__VA_ARGS__)

ApexWebSocket::ApexWebSocket()
{

}

ApexWebSocket::~ApexWebSocket()
{
    if (WebSocket.IsValid() && WebSocket->IsConnected())
    {
        // Clear the OnClosed event to prevent some access errors when the app is shutting down.
        WebSocket->OnClosed().Clear();
        WebSocket->Close();
    }
}

bool ApexWebSocket::AttemptConnect(FString Uri, FString Protocol)
{
    if (WebSocket.IsValid() && WebSocket->IsConnected())
        return false;

    PendingWebsocketRequests.Empty();

    if (!WebSocket.IsValid())
    {
        SetupSocket(WebSocket, Uri, Protocol);
    }

    WebSocket->Connect();

    return true;
}

bool ApexWebSocket::IsWebsocketConnected()
{
    if (WebSocket.IsValid() && WebSocket->IsConnected())
        return true;

    return false;
}

void ApexWebSocket::RequestAuthorizationCode()
{
    PendingWebsocketRequests.Add(ApexWebSocketRequestType::AuthorizationCode);

    if (WebSocket.IsValid() && WebSocket->IsConnected())
    {
        ProcessNextRequest();
    }
}

void ApexWebSocket::SetupSocket(TSharedPtr<IWebSocket>& Socket, FString Uri, FString Protocol)
{
    Socket = FWebSocketsModule::Get().CreateWebSocket(Uri, Protocol);

    Socket->OnConnected().AddLambda([&]() -> void {
        LogAPEX("Apex websocket connected.");
        ProcessNextRequest();
        ApexConnectedEvent.Broadcast();
    });

    Socket->OnConnectionError().AddLambda([&](const FString& Error) -> void {
        ErrorAPEX("Failed to connect with error: %s", *Error);
        CleanUpRequests();
        ApexConnectFailedEvent.Broadcast(Error);
    });

    Socket->OnClosed().AddLambda([&](int32 StatusCode, const FString& Reason, bool bWasClean) -> void {
        LogAPEX("Socket closed with status %d, for reason %s", StatusCode, *Reason);
        CleanUpRequests();
        ApexDisconnectedEvent.Broadcast(StatusCode, Reason, bWasClean);
    });

    Socket->OnMessage().AddLambda([&](const FString& Message) -> void {
        // This code will run when we receive a string message from the server.
        LogAPEX("Message received: %s", *Message);
        UVaRestSubsystem* RestSubsystem = UApexLibrary::GetRestSubsystem();
        if (!RestSubsystem)
        {
            ErrorAPEX("Unable to parse message.");
            return;
        }

        UVaRestJsonObject* MessageJsonObject = RestSubsystem->DecodeJsonObject(Message);
        if (MessageJsonObject->HasField("auth_code"))
        {
            HandleAuthCodeReceived(MessageJsonObject);
        }

        if (MessageJsonObject->HasField("token"))
        {
            HandleAuthenticateComplete(MessageJsonObject);
        }
    });

    Socket->OnRawMessage().AddLambda([&](const void* Data, SIZE_T Size, SIZE_T BytesRemaining) -> void {
        // This code will run when we receive a raw (binary) message from the server.
        LogAPEX("A raw message was received.");
    });

    Socket->OnMessageSent().AddLambda([&](const FString& MessageString) -> void {
        LogAPEX("Apex message sent.");
        ProcessNextRequest();
    });
}

void ApexWebSocket::ProcessNextRequest()
{
    if (PendingWebsocketRequests.Num() > 0)
    {
        auto NextRequestType = PendingWebsocketRequests.Pop();

        switch (NextRequestType)
        {
        case ApexWebSocketRequestType::AuthorizationCode:
            {
                SendData("{\"action\": \"authcode\" }");
            }
            break;
        default:
            ErrorAPEX("Invalid request type submitted of type %d", NextRequestType);
        }
    }
}

void ApexWebSocket::HandleAuthCodeReceived(UVaRestJsonObject* JsonObject)
{
    FAPEXAuthenticationCode APEXAuthCode;
    APEXAuthCode.FromJsonObject(JsonObject->GetRootObject());
    ApexAuthCodeRetrievedEvent.Broadcast(APEXAuthCode.AuthCode);
}

void ApexWebSocket::HandleAuthenticateComplete(UVaRestJsonObject* JsonObject)
{
    ApexAuthenticateCompleteEvent.Broadcast(JsonObject);
}

void ApexWebSocket::CleanUpRequests()
{
    PendingWebsocketRequests.Empty();
}

void ApexWebSocket::SendData(FString Data)
{
    if (!WebSocket->IsConnected())
    {
        return;
    }
    
    WebSocket->Send(Data);
}

#undef FatalAPEX
#undef ErrorAPEX
#undef WarnAPEX
#undef LogAPEX
