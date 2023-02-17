// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ApexSDKDefines.h"
#include "IWebSocket.h"

class ApexWebSocket
{
private:
    ApexWebSocket();

public:
    ApexWebSocket(FString Uri, FString Protocol = "ws");
    ~ApexWebSocket();

    DECLARE_EVENT_OneParam(ApexWebSocket, FApexSocketAuthenticate, const FString&);
    FApexSocketAuthenticate& OnApexAuthenticate();
private:
    TSharedPtr<IWebSocket> Socket;
};