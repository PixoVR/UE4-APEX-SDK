#pragma once

// Request builders and response normalizers for each platform call. These are
// pure functions so the protocol can be tested without a client or a network.

#include "JsonHelpers.hpp"
#include "Transport.hpp"
#include "apex/Apex.h"

#include <cstdint>
#include <string>
#include <utility>

namespace apex
{

// The two hosts of an environment, and the session event route.
class PlatformHosts
{
public:
	// Headset API (modules.apex...): /ping, /login, /access, session events. Still
	// needed because Platform API login does not cover every login type yet.
	const std::string& GetHeadsetApiUrl() const
	{
		return m_headsetApiUrl;
	}
	void SetHeadsetApiUrl(std::string headsetApiUrl)
	{
		m_headsetApiUrl = std::move(headsetApiUrl);
	}

	// Platform API (apex...): /v2/query (GraphQL), /v2/auth, /heartbeat.
	const std::string& GetPlatformApiUrl() const
	{
		return m_platformApiUrl;
	}
	void SetPlatformApiUrl(std::string platformApiUrl)
	{
		m_platformApiUrl = std::move(platformApiUrl);
	}

	// Session event route on the Headset API.
	const std::string& GetEventPath() const
	{
		return m_eventPath;
	}
	void SetEventPath(std::string eventPath)
	{
		m_eventPath = std::move(eventPath);
	}

private:
	std::string m_headsetApiUrl;
	std::string m_platformApiUrl;
	std::string m_eventPath;
};

// What one platform call produced, before it becomes an ApexResult.
class CallOutcome
{
public:
	static CallOutcome Failure(EApexError error, int httpStatus, std::string message)
	{
		CallOutcome failure;
		failure.SetError(error);
		failure.SetHttpStatus(httpStatus);
		failure.SetErrorMessage(std::move(message));
		failure.SetPayload(Json());
		return failure;
	}

	EApexError GetError() const
	{
		return m_error;
	}
	void SetError(EApexError error)
	{
		m_error = error;
	}
	bool Succeeded() const
	{
		return m_error == APEX_OK;
	}

	// 0 when no HTTP response was received.
	int GetHttpStatus() const
	{
		return m_httpStatus;
	}
	void SetHttpStatus(int httpStatus)
	{
		m_httpStatus = httpStatus;
	}

	const std::string& GetErrorMessage() const
	{
		return m_message;
	}
	void SetErrorMessage(std::string message)
	{
		m_message = std::move(message);
	}

	// The normalized JSON a successful call returns; null on failure.
	const Json& GetPayload() const
	{
		return m_payload;
	}
	void SetPayload(Json payload)
	{
		m_payload = std::move(payload);
	}

private:
	EApexError m_error = APEX_OK;
	int m_httpStatus = 0;
	std::string m_message;
	Json m_payload = Json::object();
};

std::string JoinBaseUrlAndPath(const std::string& baseUrl, const std::string& path);
std::string PercentEncode(const std::string& text);

namespace endpoints
{

HttpRequest MakePingRequest(const PlatformHosts& hosts);
HttpRequest MakeLoginRequest(const PlatformHosts& hosts, const std::string& username, const std::string& password);
HttpRequest MakeTokenLoginRequest(const PlatformHosts& hosts, const std::string& userToken);
HttpRequest MakeQuickIdUsersRequest(const PlatformHosts& hosts, const std::string& serialNumber);
HttpRequest MakeQuickIdLoginRequest(const PlatformHosts& hosts, const std::string& serialNumber,
									const std::string& username);
// userId < 0 asks for a code for the token's own user.
HttpRequest MakeLoginCodeRequest(const PlatformHosts& hosts, const std::string& userToken, int64_t userId);
HttpRequest MakeUserModulesRequest(const PlatformHosts& hosts, const std::string& userToken, int64_t userId);
HttpRequest MakeModuleAccessRequest(const PlatformHosts& hosts, int64_t userId, int64_t moduleId,
									const std::string& serialNumber);
HttpRequest MakeModuleListRequest(const PlatformHosts& hosts, const std::string& userToken, int64_t userId);
HttpRequest MakeSessionEventRequest(const PlatformHosts& hosts, const std::string& userToken, const Json& eventBody);
HttpRequest MakeHeartbeatRequest(const PlatformHosts& hosts, const std::string& userToken, int64_t sessionId);

CallOutcome ParsePingResponse(const HttpResponse& response);
// The three login calls all produce the user schema (spec/schema/user.json).
CallOutcome ParseLoginResponse(const HttpResponse& response);
CallOutcome ParseTokenLoginResponse(const HttpResponse& response, const std::string& userToken);
CallOutcome ParseQuickIdLoginResponse(const HttpResponse& response);
CallOutcome ParseQuickIdUsersResponse(const HttpResponse& response);
CallOutcome ParseLoginCodeResponse(const HttpResponse& response);
CallOutcome ParseUserModulesResponse(const HttpResponse& response);
CallOutcome ParseModuleAccessResponse(const HttpResponse& response);
// The module list. Its payload holds the platform's modules as they came
// ({"modules": [...]}), so they can be filtered and paged without asking again.
CallOutcome ParseModuleCatalogResponse(const HttpResponse& response);
// Every lifecycle flag: all versions but Archived ones.
constexpr uint32_t kEveryReturnedLifecycle = APEX_LIFECYCLE_RELEASED | APEX_LIFECYCLE_DEVELOPMENT | APEX_LIFECYCLE_QA |
											 APEX_LIFECYCLE_UAT | APEX_LIFECYCLE_OTHER;
// Normalizes catalog modules (spec/schema/module_list.json), keeping versions
// built for platformName (empty: any) whose lifecycle is in lifecycleMask, and
// modules left with at least one version.
Json FilterModules(const Json& catalogModules, const std::string& platformName, uint32_t lifecycleMask);
void SortModules(Json& normalizedModules, EApexModuleSort sortOrder);
// One page of normalized modules (spec/schema/module_page.json).
Json BuildModulePage(const Json& normalizedModules, int32_t pageIndex, int32_t pageSize);
// The catalog filtered to platformName (empty: any) and every returned lifecycle.
CallOutcome ParseModuleListResponse(const HttpResponse& response, const std::string& platformName);
CallOutcome ParseSessionJoinResponse(const HttpResponse& response);
// Event, complete and heartbeat: success carries an empty payload.
CallOutcome ParseAcknowledgementResponse(const HttpResponse& response);

}  // namespace endpoints
}  // namespace apex
