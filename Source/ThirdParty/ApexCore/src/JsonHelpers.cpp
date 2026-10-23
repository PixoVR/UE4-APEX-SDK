#include "JsonHelpers.hpp"

#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace apex
{

namespace
{

char ToLowerAscii(char character)
{
	if (character >= 'A' && character <= 'Z')
	{
		return static_cast<char>(character - 'A' + 'a');
	}

	return character;
}

}  // namespace

bool EqualsIgnoringCase(std::string_view firstText, std::string_view secondText)
{
	if (firstText.size() != secondText.size())
	{
		return false;
	}

	for (size_t characterIndex = 0; characterIndex < firstText.size(); ++characterIndex)
	{
		if (ToLowerAscii(firstText[characterIndex]) != ToLowerAscii(secondText[characterIndex]))
		{
			return false;
		}
	}

	return true;
}

const Json* FindMemberIgnoringCase(const Json& jsonObject, std::string_view memberName)
{
	if (!jsonObject.is_object())
	{
		return nullptr;
	}

	const auto exactMatch = jsonObject.find(std::string(memberName));
	if (exactMatch != jsonObject.end())
	{
		return &*exactMatch;
	}

	for (auto member = jsonObject.begin(); member != jsonObject.end(); ++member)
	{
		if (EqualsIgnoringCase(member.key(), memberName))
		{
			return &member.value();
		}
	}

	return nullptr;
}

std::optional<std::string> GetString(const Json& jsonObject, std::string_view memberName)
{
	const Json* memberValue = FindMemberIgnoringCase(jsonObject, memberName);
	if (memberValue == nullptr || memberValue->is_null())
	{
		return std::nullopt;
	}

	if (memberValue->is_string())
	{
		return memberValue->get<std::string>();
	}

	if (memberValue->is_number() || memberValue->is_boolean())
	{
		return SerializeJson(*memberValue);
	}

	return std::nullopt;
}

std::optional<int64_t> ConvertToInteger(const Json& jsonValue)
{
	if (jsonValue.is_number_unsigned())
	{
		const uint64_t unsignedNumber = jsonValue.get<uint64_t>();
		if (unsignedNumber > static_cast<uint64_t>(INT64_MAX))
		{
			return std::nullopt;
		}

		return static_cast<int64_t>(unsignedNumber);
	}

	if (jsonValue.is_number_integer())
	{
		return jsonValue.get<int64_t>();
	}

	if (jsonValue.is_number_float())
	{
		// Only whole numbers in range; casting anything else is undefined.
		const double floatingNumber = jsonValue.get<double>();
		if (!(floatingNumber >= -9.2e18 && floatingNumber <= 9.2e18) || floatingNumber != std::floor(floatingNumber))
		{
			return std::nullopt;
		}

		return static_cast<int64_t>(floatingNumber);
	}

	if (jsonValue.is_string())
	{
		const std::string numberText = jsonValue.get<std::string>();
		if (numberText.empty())
		{
			return std::nullopt;
		}

		char* parseEnd = nullptr;
		errno = 0;
		const long long parsedNumber = std::strtoll(numberText.c_str(), &parseEnd, 10);
		if (errno != 0 || parseEnd == nullptr || *parseEnd != '\0')
		{
			return std::nullopt;
		}

		return static_cast<int64_t>(parsedNumber);
	}

	return std::nullopt;
}

std::optional<int64_t> GetInteger(const Json& jsonObject, std::string_view memberName)
{
	const Json* memberValue = FindMemberIgnoringCase(jsonObject, memberName);
	if (memberValue == nullptr)
	{
		return std::nullopt;
	}

	return ConvertToInteger(*memberValue);
}

std::optional<double> GetNumber(const Json& jsonObject, std::string_view memberName)
{
	const Json* memberValue = FindMemberIgnoringCase(jsonObject, memberName);
	if (memberValue == nullptr || memberValue->is_null())
	{
		return std::nullopt;
	}

	if (memberValue->is_number())
	{
		return memberValue->get<double>();
	}

	if (memberValue->is_string())
	{
		const std::string numberText = memberValue->get<std::string>();
		char* parseEnd = nullptr;
		const double parsedNumber = std::strtod(numberText.c_str(), &parseEnd);
		if (!numberText.empty() && parseEnd != nullptr && *parseEnd == '\0')
		{
			return parsedNumber;
		}
	}

	return std::nullopt;
}

std::optional<bool> GetBoolean(const Json& jsonObject, std::string_view memberName)
{
	const Json* memberValue = FindMemberIgnoringCase(jsonObject, memberName);
	if (memberValue == nullptr || memberValue->is_null())
	{
		return std::nullopt;
	}

	if (memberValue->is_boolean())
	{
		return memberValue->get<bool>();
	}

	if (memberValue->is_string())
	{
		const std::string booleanText = memberValue->get<std::string>();
		if (EqualsIgnoringCase(booleanText, "true"))
		{
			return true;
		}

		if (EqualsIgnoringCase(booleanText, "false"))
		{
			return false;
		}
	}

	if (memberValue->is_number_integer())
	{
		return memberValue->get<int64_t>() != 0;
	}

	return std::nullopt;
}

std::optional<std::string> GetPlatformError(const Json& responseBody)
{
	if (!responseBody.is_object())
	{
		return std::nullopt;
	}

	const Json* graphQlErrors = FindMemberIgnoringCase(responseBody, "errors");
	if (graphQlErrors != nullptr && graphQlErrors->is_array() && !graphQlErrors->empty())
	{
		const std::optional<std::string> firstErrorMessage = GetString((*graphQlErrors)[0], "message");
		if (firstErrorMessage && !firstErrorMessage->empty())
		{
			return *firstErrorMessage;
		}

		return std::string("Unknown GraphQL error");
	}

	const Json* restErrorFlag = FindMemberIgnoringCase(responseBody, "Error");
	if (restErrorFlag == nullptr || restErrorFlag->is_null())
	{
		return std::nullopt;
	}

	bool platformReportedError = false;
	if (restErrorFlag->is_boolean())
	{
		platformReportedError = restErrorFlag->get<bool>();
	}
	else if (restErrorFlag->is_string())
	{
		const std::string errorFlagText = restErrorFlag->get<std::string>();
		platformReportedError = !errorFlagText.empty() && !EqualsIgnoringCase(errorFlagText, "false");
	}

	if (!platformReportedError)
	{
		return std::nullopt;
	}

	const std::optional<std::string> errorMessage = GetString(responseBody, "Message");
	if (errorMessage && !errorMessage->empty())
	{
		return *errorMessage;
	}

	if (restErrorFlag->is_string() && !EqualsIgnoringCase(restErrorFlag->get<std::string>(), "true"))
	{
		return restErrorFlag->get<std::string>();
	}

	return std::string("The platform reported an error");
}

bool ParseJsonObject(const char* jsonText, Json& parsedObject)
{
	if (jsonText == nullptr || *jsonText == '\0')
	{
		parsedObject = Json::object();
		return true;
	}

	Json parsedValue = Json::parse(jsonText, nullptr, false);
	if (parsedValue.is_discarded())
	{
		return false;
	}

	if (parsedValue.is_null())
	{
		parsedObject = Json::object();
		return true;
	}

	if (!parsedValue.is_object())
	{
		return false;
	}

	parsedObject = std::move(parsedValue);
	return true;
}

std::string SerializeJson(const Json& jsonValue)
{
	return jsonValue.dump(-1, ' ', false, Json::error_handler_t::replace);
}

size_t CopyToCallerBuffer(const std::string& text, char* callerBuffer, size_t callerBufferSize)
{
	if (callerBuffer != nullptr && callerBufferSize > 0)
	{
		const size_t copiedLength = text.size() < callerBufferSize - 1 ? text.size() : callerBufferSize - 1;
		std::memcpy(callerBuffer, text.data(), copiedLength);
		callerBuffer[copiedLength] = '\0';
	}

	return text.size();
}

}  // namespace apex
