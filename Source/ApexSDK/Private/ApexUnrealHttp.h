// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include "apex/Apex.h"

/**
 * Sends the core's requests through Unreal's HTTP module (ApexConfig.HttpSendCallback), for
 * platforms where the core has no HTTP backend of its own (macOS, Linux, iOS) or when the settings
 * ask for it. Requests complete on the game thread, where the client lives.
 *
 * Must outlive the client: call Shutdown before destroying the client, so no completion reaches it
 * afterwards.
 */
class FApexUnrealHttp
{
public:
	~FApexUnrealHttp();

	/** Fills the config's HTTP override with this object. */
	void Configure(ApexConfig& config);

	/** The client to report completions to, once ApexClientCreate returned it. */
	void SetClient(ApexClient* client);

	/** Cancels requests in flight without reporting them. */
	void Shutdown();

private:
	static void APEX_CALL SendRequest(void* userData, const ApexHttpRequest* request);
	void Send(const ApexHttpRequest& request);

	ApexClient* m_client = nullptr;
	TMap<uint64, TSharedRef<IHttpRequest, ESPMode::ThreadSafe>> m_requestsInFlight;
};
