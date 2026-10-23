// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#include "ApexAuthorizationCodeSocket.h"

#include "ApexJson.h"
#include "Async/Async.h"
#include "IWebSocket.h"
#include "WebSocketsModule.h"

DEFINE_LOG_CATEGORY_STATIC(LogApexAuthorizationCode, Log, All);

FApexAuthorizationCodeSocket::~FApexAuthorizationCodeSocket()
{
	Close();
}

void FApexAuthorizationCodeSocket::RequestCode(const FString& webSocketUrl, FCodeReceived onCodeReceived,
											   FTokenReceived onTokenReceived, FFailed onFailed)
{
	m_onCodeReceived = onCodeReceived;
	m_onTokenReceived = onTokenReceived;
	m_onFailed = onFailed;

	if (IsConnected() && m_webSocketUrl == webSocketUrl)
	{
		SendCodeRequest();
		return;
	}

	Close();
	m_webSocketUrl = webSocketUrl;
	m_webSocket = FWebSocketsModule::Get().CreateWebSocket(webSocketUrl, TEXT("wss"));
	m_webSocket->OnConnected().AddLambda([this]()
	{
		UE_LOG(LogApexAuthorizationCode, Log, TEXT("Connected; asking for a code."));
		SendCodeRequest();
	});
	m_webSocket->OnConnectionError().AddLambda([this](const FString& errorText)
	{
		UE_LOG(LogApexAuthorizationCode, Error, TEXT("Failed to connect: %s"), *errorText);
		RunLater([this, errorText]()
		{
			m_onFailed.ExecuteIfBound(errorText);
		});
	});
	m_webSocket->OnClosed().AddLambda([](int32 statusCode, const FString& reason, bool /*wasClean*/)
	{
		UE_LOG(LogApexAuthorizationCode, Log, TEXT("Closed (%d): %s"), statusCode, *reason);
	});
	m_webSocket->OnMessage().AddLambda([this](const FString& messageText)
	{
		RunLater([this, messageText]()
		{
			HandleMessage(messageText);
		});
	});
	m_webSocket->Connect();
}

bool FApexAuthorizationCodeSocket::IsConnected() const
{
	return m_webSocket.IsValid() && m_webSocket->IsConnected();
}

void FApexAuthorizationCodeSocket::Close()
{
	if (!m_webSocket.IsValid())
	{
		return;
	}

	// Unbound first: a close during shutdown must not call back into a destroyed subsystem.
	m_webSocket->OnConnected().Clear();
	m_webSocket->OnConnectionError().Clear();
	m_webSocket->OnClosed().Clear();
	m_webSocket->OnMessage().Clear();
	if (m_webSocket->IsConnected())
	{
		m_webSocket->Close();
	}

	m_webSocket.Reset();
}

void FApexAuthorizationCodeSocket::RunLater(TFunction<void()> work)
{
	TWeakPtr<bool, ESPMode::ThreadSafe> lifetimeToken = m_lifetimeToken;
	AsyncTask(ENamedThreads::GameThread, [lifetimeToken, work = MoveTemp(work)]()
	{
		if (lifetimeToken.IsValid())
		{
			work();
		}
	});
}

FString FApexAuthorizationCodeSocket::MakeWebSocketUrl(const FString& headsetApiUrl)
{
	FString hostAndPath = headsetApiUrl;
	const int32 schemeEnd = hostAndPath.Find(TEXT("://"));
	if (schemeEnd != INDEX_NONE)
	{
		hostAndPath = hostAndPath.RightChop(schemeEnd + 3);
	}

	int32 pathStart = INDEX_NONE;
	const FString host = hostAndPath.FindChar(TEXT('/'), pathStart) ? hostAndPath.Left(pathStart) : hostAndPath;
	return TEXT("wss://") + host + TEXT("/ws");
}

void FApexAuthorizationCodeSocket::SendCodeRequest()
{
	if (IsConnected())
	{
		m_webSocket->Send(TEXT("{\"action\":\"authcode\"}"));
	}
}

void FApexAuthorizationCodeSocket::HandleMessage(const FString& messageText)
{
	const TSharedPtr<FJsonObject> messageObject = ApexJson::ParseObject(messageText);
	if (!messageObject.IsValid())
	{
		return;
	}

	const FString authorizationCode = ApexJson::GetString(messageObject, TEXT("auth_code"));
	if (!authorizationCode.IsEmpty())
	{
		m_onCodeReceived.ExecuteIfBound(authorizationCode);
	}

	FString userToken = ApexJson::GetString(messageObject, TEXT("token"));
	if (userToken.IsEmpty())
	{
		userToken = ApexJson::GetString(messageObject, TEXT("Token"));
	}

	if (!userToken.IsEmpty())
	{
		// One login per code: later messages are not passed on.
		FTokenReceived onTokenReceived = m_onTokenReceived;
		m_onCodeReceived.Unbind();
		m_onTokenReceived.Unbind();
		m_onFailed.Unbind();
		onTokenReceived.ExecuteIfBound(userToken);
	}
}
