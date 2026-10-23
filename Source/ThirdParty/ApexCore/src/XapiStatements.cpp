#include "XapiStatements.hpp"

#include "SessionInfo.hpp"

#include <cctype>
#include <cmath>
#include <cstdio>

namespace apex
{
namespace xapi
{
namespace
{

constexpr int64_t kTicksPerSecond = 10000000;  // .NET ticks are 100 ns
constexpr int64_t kSecondsPerDay = 86400;

// Year, month and day of a count of days since 1970-01-01 (Howard Hinnant's civil_from_days).
void GetCivilDate(int64_t daysSinceEpoch, int64_t& year, int64_t& month, int64_t& day)
{
	const int64_t shiftedDays = daysSinceEpoch + 719468;
	const int64_t era = (shiftedDays >= 0 ? shiftedDays : shiftedDays - 146096) / 146097;
	const int64_t dayOfEra = shiftedDays - era * 146097;
	const int64_t yearOfEra = (dayOfEra - dayOfEra / 1460 + dayOfEra / 36524 - dayOfEra / 146096) / 365;
	const int64_t dayOfYear = dayOfEra - (365 * yearOfEra + yearOfEra / 4 - yearOfEra / 100);
	const int64_t monthIndex = (5 * dayOfYear + 2) / 153;
	day = dayOfYear - (153 * monthIndex + 2) / 5 + 1;
	month = monthIndex < 10 ? monthIndex + 3 : monthIndex - 9;
	year = yearOfEra + era * 400 + (month <= 2 ? 1 : 0);
}

std::string GetText(const Json& jsonObject, const char* memberName)
{
	return GetString(jsonObject, memberName).value_or("");
}

double GetNumberOrZero(const Json& jsonObject, const char* memberName)
{
	return GetNumber(jsonObject, memberName).value_or(0.0);
}

bool IsModuleStatement(const Json& moduleJson)
{
	return moduleJson.is_object() && moduleJson.contains("verb");
}

// "https://pixovr.com/xapi/objects/<moduleId>/<scenarioId>", plus "/<target>" when there is one.
std::string MakeObjectId(int32_t moduleId, const std::string& scenarioId, const std::string& targetObject)
{
	std::string objectId = std::string(kObjectPrefix) + std::to_string(moduleId) + "/" + scenarioId;
	if (!targetObject.empty())
	{
		objectId += "/" + ToIdentifierPart(targetObject);
	}

	return objectId;
}

Json MakeVerb(const std::string& verbId, const std::string& verbDisplay)
{
	return {{"id", verbId}, {"display", {{"en", verbDisplay}}}};
}

Json MakeActivity(const std::string& activityId)
{
	return {{"objectType", "Activity"}, {"id", activityId}};
}

// The module's extensions, then the standard ones, which win on a clash.
Json MergeExtensions(Json moduleExtensions, const Json& standardExtensions)
{
	if (!moduleExtensions.is_object())
	{
		moduleExtensions = Json::object();
	}

	for (const auto& [extensionKey, extensionValue] : standardExtensions.items())
	{
		moduleExtensions[extensionKey] = extensionValue;
	}

	return moduleExtensions;
}

Json MakeContext(const StatementContext& context, const Json& moduleExtensions)
{
	return {
		{"registration", context.GetRegistration()},
		{"revision", context.GetRevision()},
		{"platform", context.GetPlatform()},
		{"extensions", MergeExtensions(moduleExtensions, context.GetStandardExtensions())},
	};
}

Json MakeResult(const Json& completeData)
{
	Json result = {
		{"completion", GetBoolean(completeData, "complete").value_or(true)},
		{"success", GetBoolean(completeData, "success").value_or(false)},
		{"duration", FormatDuration(GetNumberOrZero(completeData, "durationSeconds"))},
		{"score",
		 {
			 {"scaled", GetNumberOrZero(completeData, "scaledScore")},
			 {"raw", GetNumberOrZero(completeData, "score")},
			 {"min", GetNumberOrZero(completeData, "minimumScore")},
			 {"max", GetNumberOrZero(completeData, "maximumScore")},
		 }},
	};
	const Json* resultData = FindMemberIgnoringCase(completeData, "data");
	if (resultData != nullptr && !resultData->is_null())
	{
		result["extensions"] = MakeExtensions(*resultData);
	}

	return result;
}

// A statement the module built: the actor, session context and stamp are the SDK's.
Json CompleteModuleStatement(const StatementContext& context, Json statement)
{
	const Json* moduleContext = FindMemberIgnoringCase(statement, "context");
	Json mergedContext = moduleContext != nullptr && moduleContext->is_object() ? *moduleContext : Json::object();
	const Json* moduleExtensions = FindMemberIgnoringCase(mergedContext, "extensions");
	const Json sessionContext = MakeContext(context, moduleExtensions != nullptr ? *moduleExtensions : Json::object());
	for (const auto& [contextKey, contextValue] : sessionContext.items())
	{
		mergedContext[contextKey] = contextValue;
	}

	statement["context"] = std::move(mergedContext);
	statement["actor"] = {{"objectType", "Agent"}, {"mbox", context.GetActorEmail()}};
	if (!statement.contains("id"))
	{
		statement["id"] = context.GetStatementId();
	}

	if (!statement.contains("timestamp"))
	{
		statement["timestamp"] = context.GetTimestamp();
	}

	if (!statement.contains("version"))
	{
		statement["version"] = kStatementVersion;
	}

	return statement;
}

}  // namespace

std::string FormatTimestamp(std::chrono::system_clock::time_point timePoint)
{
	const int64_t ticksSinceEpoch =
		std::chrono::duration_cast<std::chrono::nanoseconds>(timePoint.time_since_epoch()).count() / 100;
	const int64_t ticksPerDay = kSecondsPerDay * kTicksPerSecond;
	int64_t daysSinceEpoch = ticksSinceEpoch / ticksPerDay;
	int64_t ticksOfDay = ticksSinceEpoch % ticksPerDay;
	if (ticksOfDay < 0)
	{
		ticksOfDay += ticksPerDay;
		--daysSinceEpoch;
	}

	int64_t year = 0;
	int64_t month = 0;
	int64_t day = 0;
	GetCivilDate(daysSinceEpoch, year, month, day);
	const int64_t secondOfDay = ticksOfDay / kTicksPerSecond;
	char timestampText[48];
	std::snprintf(timestampText, sizeof(timestampText), "%04lld-%02lld-%02lldT%02lld:%02lld:%02lld.%07lldZ",
				  static_cast<long long>(year), static_cast<long long>(month), static_cast<long long>(day),
				  static_cast<long long>(secondOfDay / 3600), static_cast<long long>(secondOfDay / 60 % 60),
				  static_cast<long long>(secondOfDay % 60), static_cast<long long>(ticksOfDay % kTicksPerSecond));
	return timestampText;
}

std::string FormatDuration(double durationSeconds)
{
	if (!std::isfinite(durationSeconds))
	{
		durationSeconds = 0.0;
	}

	int64_t totalTicks = std::llround(durationSeconds * static_cast<double>(kTicksPerSecond));
	std::string durationText;
	if (totalTicks < 0)
	{
		durationText += '-';
		totalTicks = -totalTicks;
	}

	const int64_t totalSeconds = totalTicks / kTicksPerSecond;
	const int64_t fractionTicks = totalTicks % kTicksPerSecond;
	const int64_t dayCount = totalSeconds / kSecondsPerDay;
	const int64_t hourCount = totalSeconds / 3600 % 24;
	const int64_t minuteCount = totalSeconds / 60 % 60;
	const int64_t secondCount = totalSeconds % 60;

	durationText += 'P';
	if (dayCount > 0)
	{
		durationText += std::to_string(dayCount) + "D";
	}

	if (hourCount > 0 || minuteCount > 0 || secondCount > 0 || fractionTicks > 0)
	{
		durationText += 'T';
		if (hourCount > 0)
		{
			durationText += std::to_string(hourCount) + "H";
		}

		if (minuteCount > 0)
		{
			durationText += std::to_string(minuteCount) + "M";
		}

		if (secondCount > 0 || fractionTicks > 0)
		{
			durationText += std::to_string(secondCount);
			if (fractionTicks > 0)
			{
				char fractionText[16];
				std::snprintf(fractionText, sizeof(fractionText), "%07lld", static_cast<long long>(fractionTicks));
				std::string fractionDigits = fractionText;
				while (!fractionDigits.empty() && fractionDigits.back() == '0')
				{
					fractionDigits.pop_back();
				}

				durationText += "." + fractionDigits;
			}

			durationText += 'S';
		}
	}
	else if (dayCount == 0)
	{
		durationText += "T0S";
	}

	return durationText;
}

std::string ToIdentifierPart(const std::string& text)
{
	std::string identifierPart;
	identifierPart.reserve(text.size());
	for (const unsigned char character : text)
	{
		identifierPart += character == ' ' ? '_' : static_cast<char>(std::tolower(character));
	}

	return identifierPart;
}

Json MakeExtensions(const Json& moduleData)
{
	Json extensions = Json::object();
	if (moduleData.is_null())
	{
		return extensions;
	}

	if (!moduleData.is_object())
	{
		extensions[std::string(kExtensionPrefix) + "data"] = moduleData.dump();
		return extensions;
	}

	for (const auto& [memberName, memberValue] : moduleData.items())
	{
		const std::string extensionKey =
			memberName.find("://") != std::string::npos ? memberName : std::string(kExtensionPrefix) + memberName;
		extensions[extensionKey] = memberValue.is_string() ? memberValue.get<std::string>() : memberValue.dump();
	}

	return extensions;
}

Json MakeStandardExtensions(const Json& joinData, int32_t moduleId, const std::string& fallbackDeviceId,
							const std::string& fallbackDeviceModel)
{
	Json standardExtensions = {{std::string(kExtensionPrefix) + "moduleIds", std::to_string(moduleId)}};
	const auto addText = [&standardExtensions](const char* extensionName, const std::string& extensionText)
	{
		if (!extensionText.empty())
		{
			standardExtensions[std::string(kExtensionPrefix) + extensionName] = extensionText;
		}
	};

	const std::string deviceId = GetText(joinData, "deviceId");
	const std::string deviceModel = GetText(joinData, "deviceModel");
	addText("device_id", deviceId.empty() ? fallbackDeviceId : deviceId);
	addText("device_model", deviceModel.empty() ? fallbackDeviceModel : deviceModel);
	addText("sdk_version", GetText(joinData, "sdkVersion"));
	addText("sdk_source_hash", GetText(joinData, "sdkSourceHash"));
	const std::optional<bool> moduleAccessChecked = GetBoolean(joinData, "moduleAccessChecked");
	if (moduleAccessChecked)
	{
		addText("module_access_checked", *moduleAccessChecked ? "true" : "false");
	}

	addText("device_serial", GetText(joinData, "deviceSerial"));
	return standardExtensions;
}

Json MakeStatement(const char* eventType, const StatementContext& context, const Json& moduleJson)
{
	if (IsModuleStatement(moduleJson))
	{
		return CompleteModuleStatement(context, moduleJson);
	}

	const std::string scenarioId = GetText(moduleJson, "scenarioId");
	const Json* moduleData = FindMemberIgnoringCase(moduleJson, "data");
	const Json moduleExtensions = moduleData != nullptr ? MakeExtensions(*moduleData) : Json::object();
	Json statement = {
		{"actor", {{"objectType", "Agent"}, {"mbox", context.GetActorEmail()}}},
		{"id", context.GetStatementId()},
		{"timestamp", context.GetTimestamp()},
		{"version", kStatementVersion},
	};

	const std::string eventTypeText = eventType;
	if (eventTypeText == event_types::kJoined)
	{
		statement["verb"] = MakeVerb(std::string(kVerbPrefix) + "joined_session", "Joined Session");
		statement["object"] = MakeActivity(MakeObjectId(context.GetModuleId(), scenarioId, ""));
		statement["context"] = MakeContext(context, moduleExtensions);
	}
	else if (eventTypeText == event_types::kComplete)
	{
		statement["verb"] = MakeVerb(std::string(kVerbPrefix) + "completed_session", "Completed Session");
		statement["object"] = MakeActivity(MakeObjectId(context.GetModuleId(), scenarioId, ""));
		statement["context"] = MakeContext(context, Json::object());
		statement["result"] = MakeResult(moduleJson);
	}
	else
	{
		const std::string action = GetText(moduleJson, "action");
		statement["verb"] = MakeVerb(std::string(kVerbPrefix) + ToIdentifierPart(action), action);
		statement["object"] =
			MakeActivity(MakeObjectId(context.GetModuleId(), scenarioId, GetText(moduleJson, "target")));
		statement["context"] = MakeContext(context, moduleExtensions);
	}

	return statement;
}

Json MakeEventBody(const char* eventType, const StatementContext& context, const std::string& deviceId,
				   const std::string& ipAddress, Json statement, const Json& moduleJson)
{
	const std::string eventTypeText = eventType;
	Json eventBody = {
		{"uuid", context.GetRegistration()},
		{"eventType", eventTypeText},
		{"moduleId", context.GetModuleId()},
	};
	if (!deviceId.empty())
	{
		eventBody["deviceId"] = deviceId;
	}

	if (eventTypeText == event_types::kJoined && !ipAddress.empty())
	{
		eventBody["ipAddress"] = ipAddress;
	}

	if (eventTypeText == event_types::kComplete)
	{
		const Json* result = FindMemberIgnoringCase(statement, "result");
		const Json* score = result != nullptr ? FindMemberIgnoringCase(*result, "score") : nullptr;
		const Json noScore = Json::object();
		const Json& scoreObject = score != nullptr && score->is_object() ? *score : noScore;
		const bool succeeded = result != nullptr && GetBoolean(*result, "success").value_or(false);
		statement["score"] = GetNumberOrZero(scoreObject, "raw");
		statement["scoreMin"] = GetNumberOrZero(scoreObject, "min");
		statement["scoreMax"] = GetNumberOrZero(scoreObject, "max");
		statement["scoreScaled"] = GetNumberOrZero(scoreObject, "scaled");
		statement["sessionDuration"] = static_cast<int64_t>(GetNumberOrZero(moduleJson, "durationSeconds"));
		statement["lessonStatus"] = succeeded ? "passed" : "failed";
		statement["moduleName"] = std::to_string(context.GetModuleId());
	}

	eventBody["jsonData"] = std::move(statement);
	return eventBody;
}

}  // namespace xapi
}  // namespace apex
