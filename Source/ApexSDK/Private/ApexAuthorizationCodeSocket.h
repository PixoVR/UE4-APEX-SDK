// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class IWebSocket;

/**
 * Login by authorization code: the headset shows a code from the Headset API's WebSocket
 * (wss://<host>/ws), the user enters it on another device, and the socket then hands over a user
 * token. The core has no WebSocket, so this lives in the SDK, as in the Unity SDK and PixoAuth.
 */
class FApexAuthorizationCodeSocket
{
public:
	DECLARE_DELEGATE_OneParam(FCodeReceived, const FString& /*authorizationCode*/);
	DECLARE_DELEGATE_OneParam(FTokenReceived, const FString& /*userToken*/);
	DECLARE_DELEGATE_OneParam(FFailed, const FString& /*reason*/);

	~FApexAuthorizationCodeSocket();

	/** Connects if needed and asks for a code. Callbacks run on the game thread. */
	void RequestCode(const FString& webSocketUrl, FCodeReceived onCodeReceived, FTokenReceived onTokenReceived,
					 FFailed onFailed);

	bool IsConnected() const;
	void Close();

	/** wss://<host>/ws for a Headset API URL such as https://modules.apex.pixovr.com. */
	static FString MakeWebSocketUrl(const FString& headsetApiUrl);

private:
	void SendCodeRequest();
	void HandleMessage(const FString& messageText);

	TSharedPtr<IWebSocket> m_webSocket;
	FString m_webSocketUrl;
	FCodeReceived m_onCodeReceived;
	FTokenReceived m_onTokenReceived;
	FFailed m_onFailed;
};
