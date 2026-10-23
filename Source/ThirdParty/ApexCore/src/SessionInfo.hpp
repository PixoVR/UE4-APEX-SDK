#pragma once

// Bodies for the Headset API's /event route (spec/schema/session_event.json):
// {"uuid", "eventType"} as in the Unity SDK 2.0, "moduleId" on the join only,
// and the module's own bytes, base64-encoded, in "data".

#include "JsonHelpers.hpp"

#include <cstdint>
#include <string>
#include <utility>

namespace apex
{

namespace event_types
{
constexpr const char* kJoined = "PIXOVR_SESSION_JOINED";
constexpr const char* kEvent = "PIXOVR_SESSION_EVENT";
constexpr const char* kComplete = "PIXOVR_SESSION_COMPLETE";
}  // namespace event_types

class SessionInfo
{
public:
	// Chosen by the client at join and sent with every event of the session.
	const std::string& GetUuid() const
	{
		return m_uuid;
	}
	void SetUuid(std::string uuid)
	{
		m_uuid = std::move(uuid);
	}

	// Assigned by the platform's join reply; the heartbeat sends it.
	int64_t GetSessionId() const
	{
		return m_sessionId;
	}
	void SetSessionId(int64_t sessionId)
	{
		m_sessionId = sessionId;
	}

	int32_t GetModuleId() const
	{
		return m_moduleId;
	}
	void SetModuleId(int32_t moduleId)
	{
		m_moduleId = moduleId;
	}

	// EApexSessionFormat, fixed at join for the whole session.
	int32_t GetSessionFormat() const
	{
		return m_sessionFormat;
	}
	void SetSessionFormat(int32_t sessionFormat)
	{
		m_sessionFormat = sessionFormat;
	}

	// xAPI only: the standard context extensions, taken from the join data.
	const Json& GetXapiStandardExtensions() const
	{
		return m_xapiStandardExtensions;
	}
	void SetXapiStandardExtensions(Json standardExtensions)
	{
		m_xapiStandardExtensions = std::move(standardExtensions);
	}

	// xAPI only: context.platform and the body's deviceId, taken from the join data.
	const std::string& GetXapiPlatform() const
	{
		return m_xapiPlatform;
	}
	void SetXapiPlatform(std::string platform)
	{
		m_xapiPlatform = std::move(platform);
	}
	const std::string& GetXapiDeviceId() const
	{
		return m_xapiDeviceId;
	}
	void SetXapiDeviceId(std::string deviceId)
	{
		m_xapiDeviceId = std::move(deviceId);
	}

private:
	std::string m_uuid;
	int64_t m_sessionId = 0;
	int32_t m_moduleId = 0;
	int32_t m_sessionFormat = 0;
	Json m_xapiStandardExtensions = Json::object();
	std::string m_xapiPlatform;
	std::string m_xapiDeviceId;
};

// "data" is left out when moduleData is empty.
Json MakeEventBody(const char* eventType, const SessionInfo& session, const std::string& moduleData);

// Standard base64 (RFC 4648) with padding: what Go's encoding/json and
// Newtonsoft use for byte arrays.
std::string Base64Encode(const std::string& bytes);

// Random RFC 4122 version 4 UUID, lowercase with dashes (like .NET's Guid.ToString()).
std::string MakeUuid();

}  // namespace apex
