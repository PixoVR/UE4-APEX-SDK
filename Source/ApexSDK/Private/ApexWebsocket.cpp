// Fill out your copyright notice in the Description page of Project Settings.

#include "ApexWebsocket.h"
#include "WebSocketsModule.h"

DEFINE_LOG_CATEGORY_STATIC(LogApexWebSocket, Log, All);

#define LogAPEX(pmt, ...) UE_LOG(LogApexWebSocket, Log, TEXT(pmt), ##__VA_ARGS__)
#define WarnAPEX(pmt, ...) UE_LOG(LogApexWebSocket, Warning, TEXT(pmt), ##__VA_ARGS__)
#define ErrorAPEX(pmt, ...) UE_LOG(LogApexWebSocket, Error, TEXT(pmt), ##__VA_ARGS__)
#define FatalAPEX(pmt, ...) UE_LOG(LogApexWebSocket, Fatal, TEXT(pmt), ##__VA_ARGS__)

ApexWebSocket::ApexWebSocket()
{

}

ApexWebSocket::ApexWebSocket(FString Uri, FString Protocol)
{
    Socket = FWebSocketsModule::Get().CreateWebSocket(Uri, Protocol);

    Socket->OnConnected().AddLambda([]() -> void {
        LogAPEX("Apex websocket connected.");
    });

    Socket->OnConnectionError().AddLambda([](const FString& Error) -> void {
        ErrorAPEX("Failed to connect with error: %s", *Error);
    });

    Socket->OnClosed().AddLambda([](int32 StatusCode, const FString& Reason, bool bWasClean) -> void {
        LogAPEX("Socket closed with status %d, for reason %s", StatusCode, *Reason);
    });

    Socket->OnMessage().AddLambda([](const FString& Message) -> void {
        // This code will run when we receive a string message from the server.
        LogAPEX("Message received: %s", *Message);
    });

    Socket->OnRawMessage().AddLambda([](const void* Data, SIZE_T Size, SIZE_T BytesRemaining) -> void {
        // This code will run when we receive a raw (binary) message from the server.
    });

    Socket->OnMessageSent().AddLambda([](const FString& MessageString) -> void {
        // This code is called after we sent a message to the server.
    });

    // And we finally connect to the server. 
    Socket->Connect();
}

ApexWebSocket::~ApexWebSocket()
{
    if (Socket.IsValid() && Socket->IsConnected())
    {
        Socket->Close();
    }
}

#undef FatalAPEX
#undef ErrorAPEX
#undef WarnAPEX
#undef LogAPEX