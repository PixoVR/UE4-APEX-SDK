#pragma once

// xAPI session events, as the Unity SDK 1.x sent them with TinCan.NET: the
// Headset API's /event route with the statement in "jsonData". Used when
// ApexConfig.SessionFormat is APEX_SESSION_FORMAT_XAPI, until the platform's
// new session route replaces it (spec/schema/xapi_session_event.json).

#include "JsonHelpers.hpp"

#include <chrono>
#include <cstdint>
#include <string>
#include <utility>

namespace apex
{
namespace xapi
{

constexpr const char* kExtensionPrefix = "https://pixovr.com/xapi/extension/";
constexpr const char* kVerbPrefix = "https://pixovr.com/xapi/verbs/";
constexpr const char* kObjectPrefix = "https://pixovr.com/xapi/objects/";
// TinCan.NET's TCAPIVersion.latest(), which 1.x statements carried.
constexpr const char* kStatementVersion = "1.0.3";

// Who and which session a statement belongs to, and its own ID and time.
class StatementContext
{
public:
	// The user's email, sent as the actor's mbox as 1.x did (without "mailto:").
	const std::string& GetActorEmail() const
	{
		return m_actorEmail;
	}
	void SetActorEmail(std::string actorEmail)
	{
		m_actorEmail = std::move(actorEmail);
	}

	// The session UUID, sent as context.registration.
	const std::string& GetRegistration() const
	{
		return m_registration;
	}
	void SetRegistration(std::string registration)
	{
		m_registration = std::move(registration);
	}

	// The module version, sent as context.revision.
	const std::string& GetRevision() const
	{
		return m_revision;
	}
	void SetRevision(std::string revision)
	{
		m_revision = std::move(revision);
	}

	const std::string& GetPlatform() const
	{
		return m_platform;
	}
	void SetPlatform(std::string platform)
	{
		m_platform = std::move(platform);
	}

	int32_t GetModuleId() const
	{
		return m_moduleId;
	}
	void SetModuleId(int32_t moduleId)
	{
		m_moduleId = moduleId;
	}

	// Module, device and SDK details added to every statement's context extensions.
	const Json& GetStandardExtensions() const
	{
		return m_standardExtensions;
	}
	void SetStandardExtensions(Json standardExtensions)
	{
		m_standardExtensions = std::move(standardExtensions);
	}

	const std::string& GetStatementId() const
	{
		return m_statementId;
	}
	void SetStatementId(std::string statementId)
	{
		m_statementId = std::move(statementId);
	}

	const std::string& GetTimestamp() const
	{
		return m_timestamp;
	}
	void SetTimestamp(std::string timestamp)
	{
		m_timestamp = std::move(timestamp);
	}

private:
	std::string m_actorEmail;
	std::string m_registration;
	std::string m_revision;
	std::string m_platform;
	int32_t m_moduleId = 0;
	Json m_standardExtensions = Json::object();
	std::string m_statementId;
	std::string m_timestamp;
};

// UTC with seven fractional digits, as .NET writes a DateTime: "2026-10-20T12:30:45.1230000Z".
std::string FormatTimestamp(std::chrono::system_clock::time_point timePoint);

// ISO 8601 duration as .NET's XmlConvert writes a TimeSpan: "PT5M12S", "P1DT2H", "PT0S".
std::string FormatDuration(double durationSeconds);

// Lowercase with spaces as underscores, for verb and object IDs.
std::string ToIdentifierPart(const std::string& text);

// The module's data as context or result extensions: each member keyed by its
// name under kExtensionPrefix (names that are already URIs stay as they are),
// each value as text. A non-object value becomes the "data" extension.
Json MakeExtensions(const Json& moduleData);

// The standard context extensions, from the SDK's join data with fallbacks:
// moduleIds, device_id, device_model, sdk_version, sdk_source_hash,
// module_access_checked and device_serial, each left out when unknown.
Json MakeStandardExtensions(const Json& joinData, int32_t moduleId, const std::string& fallbackDeviceId,
							const std::string& fallbackDeviceModel);

// The statement for one session call. moduleJson is the SDK's session JSON
// (spec/README.md) or, when it has a "verb", a statement the module built
// itself, which keeps its verb, object and result and gets the actor, the
// session context and the standard extensions.
Json MakeStatement(const char* eventType, const StatementContext& context, const Json& moduleJson);

// The /event body: {"uuid", "eventType", "moduleId", "deviceId", "jsonData"},
// plus "ipAddress" on the join when known. The completion's jsonData also
// carries the score summary 1.x added: score, scoreMin, scoreMax, scoreScaled,
// sessionDuration, lessonStatus and moduleName.
Json MakeEventBody(const char* eventType, const StatementContext& context, const std::string& deviceId,
				   const std::string& ipAddress, Json statement, const Json& moduleJson);

}  // namespace xapi
}  // namespace apex
