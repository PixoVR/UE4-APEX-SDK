// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#include "ApexUnrealHttp.h"

#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"

FApexUnrealHttp::~FApexUnrealHttp()
{
	Shutdown();
}

void FApexUnrealHttp::Configure(ApexConfig& config)
{
	config.HttpSendCallback = &FApexUnrealHttp::SendRequest;
	config.HttpSendUserData = this;
}

void FApexUnrealHttp::SetClient(ApexClient* client)
{
	m_client = client;
}

void FApexUnrealHttp::Shutdown()
{
	for (TPair<uint64, TSharedRef<IHttpRequest, ESPMode::ThreadSafe>>& requestInFlight : m_requestsInFlight)
	{
		requestInFlight.Value->OnProcessRequestComplete().Unbind();
		requestInFlight.Value->CancelRequest();
	}

	m_requestsInFlight.Empty();
	m_client = nullptr;
}

void APEX_CALL FApexUnrealHttp::SendRequest(void* userData, const ApexHttpRequest* request)
{
	if (userData != nullptr && request != nullptr)
	{
		static_cast<FApexUnrealHttp*>(userData)->Send(*request);
	}
}

void FApexUnrealHttp::Send(const ApexHttpRequest& request)
{
	const uint64 requestId = request.RequestId;
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> httpRequest = FHttpModule::Get().CreateRequest();
	httpRequest->SetVerb(UTF8_TO_TCHAR(request.Method));
	httpRequest->SetURL(UTF8_TO_TCHAR(request.Url));
	for (size_t headerIndex = 0; headerIndex < request.HeaderCount; ++headerIndex)
	{
		httpRequest->SetHeader(UTF8_TO_TCHAR(request.Headers[headerIndex].Name),
							   UTF8_TO_TCHAR(request.Headers[headerIndex].Value));
	}

	if (request.Body != nullptr && request.BodyLength > 0)
	{
		TArray<uint8> requestBody;
		requestBody.Append(reinterpret_cast<const uint8*>(request.Body), static_cast<int32>(request.BodyLength));
		httpRequest->SetContent(requestBody);
	}

	httpRequest->OnProcessRequestComplete().BindLambda(
		[this, requestId](FHttpRequestPtr /*completedRequest*/, FHttpResponsePtr response, bool connectedSuccessfully)
	{
		m_requestsInFlight.Remove(requestId);
		if (m_client == nullptr)
		{
			return;
		}

		if (!connectedSuccessfully || !response.IsValid())
		{
			ApexHttpComplete(m_client, requestId, 0, nullptr, 0, "the request did not reach the server");
			return;
		}

		const TArray<uint8>& responseBody = response->GetContent();
		ApexHttpComplete(m_client, requestId, response->GetResponseCode(),
						 reinterpret_cast<const char*>(responseBody.GetData()), static_cast<size_t>(responseBody.Num()),
						 nullptr);
	});

	m_requestsInFlight.Add(requestId, httpRequest);
	httpRequest->ProcessRequest();
}
