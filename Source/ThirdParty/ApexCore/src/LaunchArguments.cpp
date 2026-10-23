#include "LaunchArguments.hpp"

namespace apex
{
namespace
{

constexpr int kModuleVersionPartCount = 3;

int HexDigitValue(char hexDigit)
{
	if (hexDigit >= '0' && hexDigit <= '9')
	{
		return hexDigit - '0';
	}

	if (hexDigit >= 'a' && hexDigit <= 'f')
	{
		return hexDigit - 'a' + 10;
	}

	if (hexDigit >= 'A' && hexDigit <= 'F')
	{
		return hexDigit - 'A' + 10;
	}

	return -1;
}

// Adds each "key=value" pair of "key=value&key=value" to launchArguments.
void AddKeyValuePairs(const std::string& pairsText, LaunchArguments& launchArguments)
{
	size_t pairStart = 0;
	while (pairStart <= pairsText.size())
	{
		size_t pairEnd = pairsText.find('&', pairStart);
		if (pairEnd == std::string::npos)
		{
			pairEnd = pairsText.size();
		}

		const std::string pairText = pairsText.substr(pairStart, pairEnd - pairStart);
		const size_t equalsPosition = pairText.find('=');
		if (equalsPosition != std::string::npos && equalsPosition > 0)
		{
			launchArguments[pairText.substr(0, equalsPosition)] = pairText.substr(equalsPosition + 1);
		}

		pairStart = pairEnd + 1;
	}
}

std::string FindArgument(const LaunchArguments& launchArguments, const char* key)
{
	const auto foundArgument = launchArguments.find(key);
	return foundArgument == launchArguments.end() ? std::string() : foundArgument->second;
}

const char* TargetTypeOf(const std::string& target)
{
	return target.find("://") != std::string::npos ? "url" : "app";
}

}  // namespace

std::string UrlUnescape(const std::string& escapedText)
{
	std::string unescapedText;
	unescapedText.reserve(escapedText.size());
	for (size_t characterIndex = 0; characterIndex < escapedText.size(); ++characterIndex)
	{
		if (escapedText[characterIndex] == '%' && characterIndex + 2 < escapedText.size())
		{
			const int highNibble = HexDigitValue(escapedText[characterIndex + 1]);
			const int lowNibble = HexDigitValue(escapedText[characterIndex + 2]);
			if (highNibble >= 0 && lowNibble >= 0)
			{
				unescapedText += static_cast<char>((highNibble << 4) | lowNibble);
				characterIndex += 2;
				continue;
			}
		}

		unescapedText += escapedText[characterIndex];
	}

	return unescapedText;
}

void ParseCommandLine(int argumentCount, const char* const* argumentValues, LaunchArguments& launchArguments)
{
	if (argumentValues == nullptr || argumentCount < 2)
	{
		return;
	}

	if (argumentCount == 2)
	{
		if (argumentValues[1] != nullptr)
		{
			ParseUrlArguments(argumentValues[1], launchArguments);
		}

		return;
	}

	for (int argumentIndex = 1; argumentIndex + 1 < argumentCount; ++argumentIndex)
	{
		const char* keyArgument = argumentValues[argumentIndex];
		const char* valueArgument = argumentValues[argumentIndex + 1];
		if (keyArgument == nullptr || keyArgument[0] != '-' || valueArgument == nullptr)
		{
			continue;
		}

		std::string key(keyArgument);
		key.erase(0, key.find_first_not_of('-'));
		if (!key.empty())
		{
			launchArguments[key] = valueArgument;
		}

		++argumentIndex;
	}
}

void ParseUrlArguments(const std::string& launchUrl, LaunchArguments& launchArguments)
{
	const std::string decodedUrl = UrlUnescape(launchUrl);
	const size_t queryStart = decodedUrl.find('?');
	std::string queryText = queryStart == std::string::npos ? decodedUrl : decodedUrl.substr(queryStart + 1);
	if (queryText.empty())
	{
		return;
	}

	const std::string optionalMarker = std::string(launch_keys::kOptional) + "=";
	const size_t optionalStart = queryText.find(optionalMarker);
	if (optionalStart != std::string::npos)
	{
		std::string textBeforeOptional = queryText.substr(0, optionalStart);
		const std::string textAfterMarker = queryText.substr(optionalStart + optionalMarker.size());
		size_t optionalValueEnd = textAfterMarker.rfind('}');
		if (optionalValueEnd != std::string::npos)
		{
			optionalValueEnd += 1;
		}
		else
		{
			// Not JSON: the value runs to the next argument.
			optionalValueEnd = textAfterMarker.find('&');
			if (optionalValueEnd == std::string::npos)
			{
				optionalValueEnd = textAfterMarker.size();
			}
		}

		launchArguments[launch_keys::kOptional] = textAfterMarker.substr(0, optionalValueEnd);
		if (!textBeforeOptional.empty() && textBeforeOptional.back() == '&')
		{
			textBeforeOptional.pop_back();
		}

		queryText = textBeforeOptional + textAfterMarker.substr(optionalValueEnd);
	}

	AddKeyValuePairs(queryText, launchArguments);
}

Json BuildExitTarget(const LaunchArguments& launchArguments, const std::string& userToken,
					 const std::string& nextTarget)
{
	const std::string tokenToPass = userToken.empty() ? FindArgument(launchArguments, launch_keys::kToken) : userToken;
	const std::string optionalData = FindArgument(launchArguments, launch_keys::kOptional);
	const std::string nextTargetType = TargetTypeOf(nextTarget);

	const std::string returnTarget = FindArgument(launchArguments, launch_keys::kReturnTarget);
	if (returnTarget.empty())
	{
		return {{"action", "none"}};
	}

	std::string returnTargetType = FindArgument(launchArguments, launch_keys::kTargetType);
	if (returnTargetType.empty())
	{
		returnTargetType = TargetTypeOf(returnTarget);
	}

	if (EqualsIgnoringCase(returnTargetType, "url"))
	{
		std::string queryText;
		const auto appendQueryArgument = [&queryText](const char* key, const std::string& value)
		{
			if (!queryText.empty())
			{
				queryText += '&';
			}

			queryText += key;
			queryText += '=';
			queryText += value;
		};
		if (!tokenToPass.empty())
		{
			appendQueryArgument(launch_keys::kToken, tokenToPass);
		}

		if (!optionalData.empty())
		{
			appendQueryArgument(launch_keys::kOptional, optionalData);
		}

		if (!nextTarget.empty())
		{
			appendQueryArgument(launch_keys::kReturnTarget, nextTarget);
		}

		appendQueryArgument(launch_keys::kTargetType, nextTargetType);

		std::string returnUrl = returnTarget;
		returnUrl += returnUrl.find('?') == std::string::npos ? '?' : '&';
		returnUrl += queryText;
		return {{"action", "open_url"}, {"url", returnUrl}};
	}

	Json intentExtras = Json::object();
	if (!tokenToPass.empty())
	{
		intentExtras[launch_keys::kToken] = tokenToPass;
	}

	if (!optionalData.empty())
	{
		intentExtras[launch_keys::kOptional] = optionalData;
	}

	if (!nextTarget.empty())
	{
		intentExtras[launch_keys::kReturnTarget] = nextTarget;
	}

	intentExtras[launch_keys::kTargetType] = nextTargetType;
	return {{"action", "open_app"}, {"package", returnTarget}, {"extras", intentExtras}};
}

bool IsModuleVersionValid(const std::string& moduleVersion)
{
	if (moduleVersion.empty() || moduleVersion.find_first_not_of("0123456789.") != std::string::npos)
	{
		return false;
	}

	std::string versionParts[kModuleVersionPartCount];
	int versionPartCount = 0;
	size_t partStart = 0;
	for (;;)
	{
		const size_t dotPosition = moduleVersion.find('.', partStart);
		if (versionPartCount == kModuleVersionPartCount)
		{
			return false;
		}

		versionParts[versionPartCount++] = moduleVersion.substr(
			partStart, dotPosition == std::string::npos ? std::string::npos : dotPosition - partStart);
		if (dotPosition == std::string::npos)
		{
			break;
		}

		partStart = dotPosition + 1;
	}

	if (versionPartCount != kModuleVersionPartCount)
	{
		return false;
	}

	const std::string& majorVersion = versionParts[0];
	if (majorVersion.empty() || majorVersion[0] == '0')
	{
		return false;
	}

	for (int partIndex = 1; partIndex < kModuleVersionPartCount; ++partIndex)
	{
		if (versionParts[partIndex].empty() || versionParts[partIndex].size() > 2)
		{
			return false;
		}
	}

	return true;
}

}  // namespace apex
