#pragma once

// Helpers for reading platform responses. The platform's REST endpoints were
// consumed by Newtonsoft.Json, which matches property names case-insensitively,
// so field casing in responses is not guaranteed; every lookup here ignores case.

#include <nlohmann/json.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace apex
{

using Json = nlohmann::json;

// The member of jsonObject whose name equals memberName ignoring ASCII case; an
// exact match wins. nullptr when jsonObject is not an object or has no such member.
const Json* FindMemberIgnoringCase(const Json& jsonObject, std::string_view memberName);

// Values that are absent or null read as std::nullopt. Strings holding numbers
// or booleans are accepted, since GraphQL IDs arrive as strings.
std::optional<std::string> GetString(const Json& jsonObject, std::string_view memberName);
std::optional<int64_t> GetInteger(const Json& jsonObject, std::string_view memberName);
std::optional<double> GetNumber(const Json& jsonObject, std::string_view memberName);
std::optional<bool> GetBoolean(const Json& jsonObject, std::string_view memberName);

// A whole number held as a JSON number or a numeric string, when it fits in int64_t.
std::optional<int64_t> ConvertToInteger(const Json& jsonValue);

// Returns the platform's error message when responseBody reports an error: a
// REST body with a true "Error" field, or a GraphQL body with a non-empty
// "errors" array.
std::optional<std::string> GetPlatformError(const Json& responseBody);

// Parses text that must be a JSON object. Empty or null text gives an empty
// object. Returns false for anything else.
bool ParseJsonObject(const char* jsonText, Json& parsedObject);

// Serializes jsonValue. Invalid UTF-8 in strings (from callers or the network)
// becomes U+FFFD instead of throwing.
std::string SerializeJson(const Json& jsonValue);

// Copies text into a caller's buffer using the "return the needed length" rule of
// Apex.h, and returns that length.
size_t CopyToCallerBuffer(const std::string& text, char* callerBuffer, size_t callerBufferSize);

bool EqualsIgnoringCase(std::string_view firstText, std::string_view secondText);

}  // namespace apex
