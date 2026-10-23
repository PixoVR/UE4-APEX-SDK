#include "Endpoints.hpp"

#include "GraphqlQueries.hpp"  // generated from spec/graphql

#include <algorithm>
#include <cctype>
#include <vector>

namespace apex
{

std::string JoinBaseUrlAndPath(const std::string& baseUrl, const std::string& path)
{
	std::string joinedUrl = baseUrl;
	while (!joinedUrl.empty() && joinedUrl.back() == '/')
	{
		joinedUrl.pop_back();
	}

	return joinedUrl + path;
}

std::string PercentEncode(const std::string& text)
{
	static const char* const kHexDigits = "0123456789ABCDEF";
	std::string encodedText;
	encodedText.reserve(text.size());
	for (const unsigned char character : text)
	{
		if (std::isalnum(character) || character == '-' || character == '_' || character == '.' || character == '~')
		{
			encodedText += static_cast<char>(character);
		}
		else
		{
			encodedText += '%';
			encodedText += kHexDigits[character >> 4];
			encodedText += kHexDigits[character & 0x0F];
		}
	}

	return encodedText;
}

namespace endpoints
{
namespace
{

void AddAuthorization(HttpRequest& request, const std::string& userToken)
{
	if (!userToken.empty())
	{
		request.AddHeader("Authorization", "Bearer " + userToken);
	}
}

HttpRequest MakeGetRequest(const std::string& baseUrl, const std::string& path,
						   const std::string& userToken = std::string())
{
	HttpRequest request;
	request.SetMethod("GET");
	request.SetUrl(JoinBaseUrlAndPath(baseUrl, path));
	request.AddHeader("Accept", "application/json");
	AddAuthorization(request, userToken);
	return request;
}

HttpRequest MakePostRequest(const std::string& baseUrl, const std::string& path, const Json& requestBody,
							const std::string& userToken = std::string())
{
	HttpRequest request;
	request.SetMethod("POST");
	request.SetUrl(JoinBaseUrlAndPath(baseUrl, path));
	request.SetBody(SerializeJson(requestBody));
	request.AddHeader("Content-Type", "application/json");
	request.AddHeader("Accept", "application/json");
	AddAuthorization(request, userToken);
	return request;
}

HttpRequest MakeGraphQlRequest(const PlatformHosts& hosts, const std::string& userToken, const char* operationName,
							   const char* graphQlDocument, Json variables)
{
	const Json requestBody = {
		{"operationName", operationName}, {"variables", std::move(variables)}, {"query", graphQlDocument}};
	return MakePostRequest(hosts.GetPlatformApiUrl(), "/v2/query", requestBody, userToken);
}

bool IsSuccessStatus(int httpStatus)
{
	return httpStatus >= 200 && httpStatus < 300;
}

// Shared first steps for every response: transport errors, JSON parsing,
// platform-reported errors and HTTP status. Returns true and fills responseBody
// when the response can go on to call-specific parsing; otherwise sets outcome.
bool CheckResponse(const HttpResponse& response, Json& responseBody, CallOutcome& outcome)
{
	const int httpStatus = response.GetStatusCode();
	outcome.SetHttpStatus(httpStatus);
	if (!response.GetTransportError().empty() || httpStatus == 0)
	{
		outcome = CallOutcome::Failure(
			APEX_ERR_NETWORK, 0,
			response.GetTransportError().empty() ? std::string("No response") : response.GetTransportError());
		return false;
	}

	responseBody = response.GetBody().empty() ? Json::object() : Json::parse(response.GetBody(), nullptr, false);
	if (responseBody.is_discarded())
	{
		if (!IsSuccessStatus(httpStatus))
		{
			outcome = CallOutcome::Failure(APEX_ERR_HTTP, httpStatus, "HTTP " + std::to_string(httpStatus));
		}
		else
		{
			outcome = CallOutcome::Failure(APEX_ERR_PARSE, httpStatus, "Response was not valid JSON");
		}

		return false;
	}

	const std::optional<std::string> platformError = GetPlatformError(responseBody);
	if (platformError)
	{
		outcome = CallOutcome::Failure(APEX_ERR_PLATFORM, httpStatus, *platformError);
		return false;
	}

	if (!IsSuccessStatus(httpStatus))
	{
		std::string failureMessage = "HTTP " + std::to_string(httpStatus);
		std::optional<std::string> failureDetail = GetString(responseBody, "Message");
		if (!failureDetail)
		{
			failureDetail = GetString(responseBody, "Msg");
		}

		if (failureDetail)
		{
			failureMessage += ": " + *failureDetail;
		}

		outcome = CallOutcome::Failure(APEX_ERR_HTTP, httpStatus, failureMessage);
		return false;
	}

	return true;
}

// A body that passed CheckResponse but lacks what a successful response must
// hold. The legacy endpoints answer some failures this way, with a message field.
CallOutcome RejectResponse(const Json& responseBody, int httpStatus, const char* fallbackMessage)
{
	for (const char* messageField : {"Message", "Msg"})
	{
		const std::optional<std::string> platformMessage = GetString(responseBody, messageField);
		if (platformMessage && !platformMessage->empty())
		{
			return CallOutcome::Failure(APEX_ERR_PLATFORM, httpStatus, *platformMessage);
		}
	}

	return CallOutcome::Failure(APEX_ERR_PARSE, httpStatus, fallbackMessage);
}

CallOutcome Succeed(int httpStatus, Json payload)
{
	CallOutcome success;
	success.SetHttpStatus(httpStatus);
	success.SetPayload(std::move(payload));
	return success;
}

Json StringOrNull(const Json& jsonObject, const char* memberName)
{
	const std::optional<std::string> memberValue = GetString(jsonObject, memberName);
	return memberValue ? Json(*memberValue) : Json();
}

Json IntegerOrNull(const Json& jsonObject, const char* memberName)
{
	const std::optional<int64_t> memberValue = GetInteger(jsonObject, memberName);
	return memberValue ? Json(*memberValue) : Json();
}

Json NumberOrNull(const Json& jsonObject, const char* memberName)
{
	const std::optional<double> memberValue = GetNumber(jsonObject, memberName);
	return memberValue ? Json(*memberValue) : Json();
}

// The array member, or nullptr when it is missing or not an array.
const Json* FindArray(const Json& jsonObject, const char* memberName)
{
	const Json* memberValue = FindMemberIgnoringCase(jsonObject, memberName);
	return memberValue != nullptr && memberValue->is_array() ? memberValue : nullptr;
}

Json NormalizeOrganization(const Json* organization)
{
	if (organization == nullptr || !organization->is_object())
	{
		return Json();
	}

	return {
		{"id", IntegerOrNull(*organization, "ID")},
		{"name", StringOrNull(*organization, "Name")},
		{"status", StringOrNull(*organization, "Status")},
		{"downloadRegion", StringOrNull(*organization, "DownloadRegion")},
		{"type", StringOrNull(*organization, "Type")},
		{"hubLogoLink", StringOrNull(*organization, "HubLogoLink")},
		{"primaryColor", StringOrNull(*organization, "PrimaryColor")},
		{"secondaryColor", StringOrNull(*organization, "SecondaryColor")},
	};
}

// The legacy login shape (LoginResponseContent in the Unity SDK).
Json NormalizeLoginUser(const Json& loginUser, const std::string& fallbackToken)
{
	Json userToken = StringOrNull(loginUser, "Token");
	if (userToken.is_null() && !fallbackToken.empty())
	{
		userToken = fallbackToken;
	}

	return {
		{"id", IntegerOrNull(loginUser, "ID")},
		{"orgId", IntegerOrNull(loginUser, "OrgId")},
		{"firstName", StringOrNull(loginUser, "First")},
		{"lastName", StringOrNull(loginUser, "Last")},
		{"username", StringOrNull(loginUser, "Username")},
		{"email", StringOrNull(loginUser, "Email")},
		{"role", StringOrNull(loginUser, "Role")},
		{"token", userToken},
		{"minimumPassingScore", NumberOrNull(loginUser, "MinimumPassingScore")},
		{"org", NormalizeOrganization(FindMemberIgnoringCase(loginUser, "Org"))},
	};
}

std::string CapitalizeFirstLetter(std::string text)
{
	if (!text.empty())
	{
		text[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(text[0])));
	}

	return text;
}

std::string GetPlatformLabel(const Json& platform)
{
	const std::optional<std::string> shortName = GetString(platform, "shortName");
	if (shortName && !shortName->empty())
	{
		return *shortName;
	}

	return GetString(platform, "name").value_or("");
}

bool IsVersionBuiltForPlatform(const Json& moduleVersion, const std::string& platformName)
{
	const Json* versionPlatforms = FindArray(moduleVersion, "platforms");
	if (versionPlatforms == nullptr)
	{
		return false;
	}

	for (const Json& versionPlatform : *versionPlatforms)
	{
		const std::optional<std::string> fullName = GetString(versionPlatform, "name");
		const std::optional<std::string> shortName = GetString(versionPlatform, "shortName");
		if ((fullName && EqualsIgnoringCase(*fullName, platformName)) ||
			(shortName && EqualsIgnoringCase(*shortName, platformName)))
		{
			return true;
		}
	}

	return false;
}

Json GetPlatformLabels(const Json& moduleVersion)
{
	Json platformLabels = Json::array();
	const Json* versionPlatforms = FindArray(moduleVersion, "platforms");
	if (versionPlatforms != nullptr)
	{
		for (const Json& versionPlatform : *versionPlatforms)
		{
			const std::string platformLabel = GetPlatformLabel(versionPlatform);
			if (!platformLabel.empty())
			{
				platformLabels.push_back(platformLabel);
			}
		}
	}

	return platformLabels;
}

// The EApexLifecycle flag of a version's lifecycle: a name the platform does
// not define, or no lifecycle, is OTHER. 0 for Archived, which is never returned.
uint32_t GetLifecycleFlag(const Json& moduleVersion)
{
	const Json* lifecycle = FindMemberIgnoringCase(moduleVersion, "lifecycle");
	const std::optional<std::string> lifecycleName =
		lifecycle != nullptr && lifecycle->is_object() ? GetString(*lifecycle, "name") : std::nullopt;
	if (!lifecycleName)
	{
		return APEX_LIFECYCLE_OTHER;
	}

	if (EqualsIgnoringCase(*lifecycleName, "Released"))
	{
		return APEX_LIFECYCLE_RELEASED;
	}

	if (EqualsIgnoringCase(*lifecycleName, "Development"))
	{
		return APEX_LIFECYCLE_DEVELOPMENT;
	}

	if (EqualsIgnoringCase(*lifecycleName, "QA"))
	{
		return APEX_LIFECYCLE_QA;
	}

	if (EqualsIgnoringCase(*lifecycleName, "UAT"))
	{
		return APEX_LIFECYCLE_UAT;
	}

	if (EqualsIgnoringCase(*lifecycleName, "Archived"))
	{
		return 0;
	}

	return APEX_LIFECYCLE_OTHER;
}

// The lifecycle "stage" in the module list schema: the EApexLifecycle name.
const char* GetLifecycleStage(uint32_t lifecycleFlag)
{
	switch (lifecycleFlag)
	{
		case APEX_LIFECYCLE_RELEASED:
			return "RELEASED";
		case APEX_LIFECYCLE_DEVELOPMENT:
			return "DEVELOPMENT";
		case APEX_LIFECYCLE_QA:
			return "QA";
		case APEX_LIFECYCLE_UAT:
			return "UAT";
		case APEX_LIFECYCLE_OTHER:
			return "OTHER";
		default:
			return "ARCHIVED";
	}
}

Json NormalizeLifecycle(const Json& moduleVersion)
{
	const Json* lifecycle = FindMemberIgnoringCase(moduleVersion, "lifecycle");
	const bool hasLifecycle = lifecycle != nullptr && lifecycle->is_object();
	return {
		{"id", hasLifecycle ? IntegerOrNull(*lifecycle, "id") : Json()},
		{"name", hasLifecycle ? StringOrNull(*lifecycle, "name") : Json()},
		{"stage", GetLifecycleStage(GetLifecycleFlag(moduleVersion))},
	};
}

Json NormalizeModuleVersions(const Json& module, const std::string& platformName, uint32_t lifecycleMask)
{
	Json moduleVersions = Json::array();
	const Json* sourceVersions = FindArray(module, "versions");
	if (sourceVersions == nullptr)
	{
		return moduleVersions;
	}

	for (const Json& sourceVersion : *sourceVersions)
	{
		if (!platformName.empty() && !IsVersionBuiltForPlatform(sourceVersion, platformName))
		{
			continue;
		}

		if ((GetLifecycleFlag(sourceVersion) & lifecycleMask) == 0)
		{
			continue;
		}

		Json controlNames = Json::array();
		const Json* sourceControls = FindArray(sourceVersion, "controls");
		if (sourceControls != nullptr)
		{
			for (const Json& sourceControl : *sourceControls)
			{
				const std::optional<std::string> controlName = GetString(sourceControl, "name");
				if (controlName)
				{
					controlNames.push_back(*controlName);
				}
			}
		}

		moduleVersions.push_back({
			{"id", IntegerOrNull(sourceVersion, "id")},
			{"version", StringOrNull(sourceVersion, "version")},
			{"package", StringOrNull(sourceVersion, "package")},
			{"createdAt", StringOrNull(sourceVersion, "createdAt")},
			{"controls", controlNames},
			{"platforms", GetPlatformLabels(sourceVersion)},
			{"lifecycle", NormalizeLifecycle(sourceVersion)},
		});
	}

	return moduleVersions;
}

Json NormalizeModulePlayer(const Json& module, const std::string& platformName)
{
	const Json* sourcePlayer = FindMemberIgnoringCase(module, "modulePlayer");
	if (sourcePlayer == nullptr || !sourcePlayer->is_object())
	{
		return Json();
	}

	Json playerVersions = Json::array();
	const Json* sourceVersions = FindArray(*sourcePlayer, "versions");
	if (sourceVersions != nullptr)
	{
		for (const Json& sourceVersion : *sourceVersions)
		{
			if (!platformName.empty() && !IsVersionBuiltForPlatform(sourceVersion, platformName))
			{
				continue;
			}

			playerVersions.push_back({
				{"id", IntegerOrNull(sourceVersion, "id")},
				{"version", StringOrNull(sourceVersion, "version")},
				{"status", StringOrNull(sourceVersion, "status")},
				{"package", StringOrNull(sourceVersion, "package")},
				{"platforms", GetPlatformLabels(sourceVersion)},
			});
		}
	}

	return {
		{"id", IntegerOrNull(*sourcePlayer, "id")},
		{"name", StringOrNull(*sourcePlayer, "name")},
		{"launchProtocol", StringOrNull(*sourcePlayer, "launchProtocol")},
		{"versions", playerVersions},
	};
}

Json NormalizeModuleLanguages(const Json& module)
{
	Json languageNames = Json::array();
	const Json* sourceLanguages = FindArray(module, "availableLanguages");
	if (sourceLanguages != nullptr)
	{
		for (const Json& sourceLanguage : *sourceLanguages)
		{
			const std::optional<std::string> displayName = GetString(sourceLanguage, "displayName");
			if (displayName && !displayName->empty())
			{
				languageNames.push_back(*displayName);
			}
		}
	}

	return languageNames;
}

Json NormalizeModule(const Json& module, const std::string& platformName, uint32_t lifecycleMask)
{
	const std::optional<std::string> industry = GetString(module, "industry");
	return {
		{"id", GetInteger(module, "id").value_or(-1)},
		{"name", StringOrNull(module, "description")},
		{"abbreviation", StringOrNull(module, "abbreviation")},
		{"externalId", StringOrNull(module, "externalId")},
		{"iconUrl", StringOrNull(module, "imageLink")},
		{"distributor", StringOrNull(module, "developer")},
		{"shortDescription", StringOrNull(module, "shortDesc")},
		{"longDescription", StringOrNull(module, "longDesc")},
		{"industry", industry ? Json(CapitalizeFirstLetter(*industry)) : Json()},
		{"details", StringOrNull(module, "details")},
		{"categories", StringOrNull(module, "categories")},
		{"isAvailable", GetBoolean(module, "isAvailable").value_or(false)},
		{"isAuthenticatedLaunch", GetBoolean(module, "isAuthenticatedLaunch").value_or(false)},
		{"languages", NormalizeModuleLanguages(module)},
		{"player", NormalizeModulePlayer(module, platformName)},
		{"versions", NormalizeModuleVersions(module, platformName, lifecycleMask)},
	};
}

// The GraphQL object at data.<fieldName>, or nullptr with outcome set to a failure.
const Json* FindGraphQlData(const Json& responseBody, const char* fieldName, int httpStatus, CallOutcome& outcome)
{
	const Json* graphQlData = FindMemberIgnoringCase(responseBody, "data");
	const Json* fieldValue = graphQlData != nullptr ? FindMemberIgnoringCase(*graphQlData, fieldName) : nullptr;
	if (fieldValue == nullptr || fieldValue->is_null())
	{
		outcome = CallOutcome::Failure(APEX_ERR_PARSE, httpStatus, "Invalid response format from server");
		return nullptr;
	}

	return fieldValue;
}

// Lowercase copy, for ordering names without regard to case.
std::string ToLowerText(const std::string& text)
{
	std::string lowerText = text;
	std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), [](unsigned char character)
	{
		return static_cast<char>(std::tolower(character));
	});
	return lowerText;
}

// The latest createdAt among a normalized module's versions; empty when none has one.
// The platform sends ISO 8601 UTC times, which order correctly as text.
std::string GetNewestVersionTime(const Json& normalizedModule)
{
	std::string newestTime;
	for (const Json& moduleVersion : normalizedModule["versions"])
	{
		const std::optional<std::string> createdAt = GetString(moduleVersion, "createdAt");
		if (createdAt && *createdAt > newestTime)
		{
			newestTime = *createdAt;
		}
	}

	return newestTime;
}

}  // namespace

HttpRequest MakePingRequest(const PlatformHosts& hosts)
{
	return MakeGetRequest(hosts.GetHeadsetApiUrl(), "/ping");
}

HttpRequest MakeLoginRequest(const PlatformHosts& hosts, const std::string& username, const std::string& password)
{
	return MakePostRequest(hosts.GetHeadsetApiUrl(), "/login", {{"Login", username}, {"Password", password}});
}

HttpRequest MakeTokenLoginRequest(const PlatformHosts& hosts, const std::string& userToken)
{
	return MakeGetRequest(hosts.GetPlatformApiUrl(), "/v2/auth/validate-signature", userToken);
}

HttpRequest MakeQuickIdUsersRequest(const PlatformHosts& hosts, const std::string& serialNumber)
{
	return MakeGetRequest(hosts.GetPlatformApiUrl(),
						  "/v2/auth/quick-id/get-users?serialNumber=" + PercentEncode(serialNumber));
}

HttpRequest MakeQuickIdLoginRequest(const PlatformHosts& hosts, const std::string& serialNumber,
									const std::string& username)
{
	return MakePostRequest(hosts.GetPlatformApiUrl(), "/v2/auth/quick-id/login",
						   {{"Username", username}, {"SerialNumber", serialNumber}});
}

HttpRequest MakeLoginCodeRequest(const PlatformHosts& hosts, const std::string& userToken, int64_t userId)
{
	Json codeInput = Json::object();
	if (userId >= 0)
	{
		codeInput["userIds"] = Json::array({userId});
	}

	return MakeGraphQlRequest(hosts, userToken, "generateAuthCode", graphql::kGenerateAuthCode, {{"input", codeInput}});
}

HttpRequest MakeUserModulesRequest(const PlatformHosts& hosts, const std::string& userToken, int64_t userId)
{
	return MakePostRequest(hosts.GetHeadsetApiUrl(), "/access/users", {{"userIds", Json::array({userId})}}, userToken);
}

HttpRequest MakeModuleAccessRequest(const PlatformHosts& hosts, int64_t userId, int64_t moduleId,
									const std::string& serialNumber)
{
	std::string accessPath = "/access/user/" + std::to_string(userId) + "/module/" + std::to_string(moduleId);
	if (!serialNumber.empty())
	{
		accessPath += "?serial=" + PercentEncode(serialNumber);
	}

	return MakeGetRequest(hosts.GetHeadsetApiUrl(), accessPath);
}

HttpRequest MakeModuleListRequest(const PlatformHosts& hosts, const std::string& userToken, int64_t userId)
{
	return MakeGraphQlRequest(hosts, userToken, "user", graphql::kUserModules, {{"id", userId}});
}

HttpRequest MakeSessionEventRequest(const PlatformHosts& hosts, const std::string& userToken, const Json& eventBody)
{
	return MakePostRequest(hosts.GetHeadsetApiUrl(), hosts.GetEventPath(), eventBody, userToken);
}

HttpRequest MakeHeartbeatRequest(const PlatformHosts& hosts, const std::string& userToken, int64_t sessionId)
{
	return MakePostRequest(hosts.GetPlatformApiUrl(), "/heartbeat/pulse", {{"sessionId", sessionId}}, userToken);
}

CallOutcome ParsePingResponse(const HttpResponse& response)
{
	return ParseAcknowledgementResponse(response);
}

CallOutcome ParseLoginResponse(const HttpResponse& response)
{
	CallOutcome outcome;
	Json responseBody;
	if (!CheckResponse(response, responseBody, outcome))
	{
		return outcome;
	}

	if (!GetString(responseBody, "Email") || !GetString(responseBody, "Token"))
	{
		return RejectResponse(responseBody, response.GetStatusCode(), "Login response had no user");
	}

	return Succeed(response.GetStatusCode(), NormalizeLoginUser(responseBody, std::string()));
}

CallOutcome ParseTokenLoginResponse(const HttpResponse& response, const std::string& userToken)
{
	CallOutcome outcome;
	Json responseBody;
	if (!CheckResponse(response, responseBody, outcome))
	{
		return outcome;
	}

	const Json* loginUser = FindMemberIgnoringCase(responseBody, "User");
	if (loginUser == nullptr || !loginUser->is_object())
	{
		return RejectResponse(responseBody, response.GetStatusCode(), "Token login response had no user");
	}

	return Succeed(response.GetStatusCode(), NormalizeLoginUser(*loginUser, userToken));
}

CallOutcome ParseQuickIdLoginResponse(const HttpResponse& response)
{
	CallOutcome outcome;
	Json responseBody;
	if (!CheckResponse(response, responseBody, outcome))
	{
		return outcome;
	}

	const Json* quickIdUser = FindMemberIgnoringCase(responseBody, "User");
	const std::optional<std::string> userToken = GetString(responseBody, "Token");
	if (quickIdUser == nullptr || !quickIdUser->is_object() || !userToken || userToken->empty())
	{
		return RejectResponse(responseBody, response.GetStatusCode(), "Quick ID login response had no user");
	}

	return Succeed(response.GetStatusCode(),
				   {
					   {"id", IntegerOrNull(*quickIdUser, "Id")},
					   {"orgId", IntegerOrNull(*quickIdUser, "OrgId")},
					   {"firstName", StringOrNull(*quickIdUser, "FirstName")},
					   {"lastName", StringOrNull(*quickIdUser, "LastName")},
					   {"username", StringOrNull(*quickIdUser, "Username")},
					   {"email", StringOrNull(*quickIdUser, "Email")},
					   {"role", StringOrNull(*quickIdUser, "Role")},
					   {"token", *userToken},
					   {"minimumPassingScore", Json()},
					   {"org", NormalizeOrganization(FindMemberIgnoringCase(*quickIdUser, "Org"))},
				   });
}

CallOutcome ParseQuickIdUsersResponse(const HttpResponse& response)
{
	CallOutcome outcome;
	Json responseBody;
	if (!CheckResponse(response, responseBody, outcome))
	{
		return outcome;
	}

	const Json* organization = FindMemberIgnoringCase(responseBody, "OrgProperties");
	const Json* sourceUsers = FindArray(responseBody, "QuickIDUsers");
	if (organization == nullptr || !organization->is_object() || sourceUsers == nullptr)
	{
		return RejectResponse(responseBody, response.GetStatusCode(), "Quick ID response had no users");
	}

	Json quickIdUsers = Json::array();
	for (const Json& sourceUser : *sourceUsers)
	{
		quickIdUsers.push_back({
			{"firstName", StringOrNull(sourceUser, "FirstName")},
			{"lastName", StringOrNull(sourceUser, "LastName")},
			{"username", StringOrNull(sourceUser, "Username")},
			{"email", StringOrNull(sourceUser, "Email")},
		});
	}

	return Succeed(response.GetStatusCode(), {
												 {"org",
												  {
													  {"name", StringOrNull(*organization, "OrgName")},
													  {"primaryColor", StringOrNull(*organization, "PrimaryColor")},
													  {"secondaryColor", StringOrNull(*organization, "SecondaryColor")},
													  {"hubLogoUrl", StringOrNull(*organization, "HubLogoURL")},
												  }},
												 {"users", quickIdUsers},
											 });
}

CallOutcome ParseLoginCodeResponse(const HttpResponse& response)
{
	CallOutcome outcome;
	Json responseBody;
	if (!CheckResponse(response, responseBody, outcome))
	{
		return outcome;
	}

	const Json* loginCode = FindGraphQlData(responseBody, "generateAuthCode", response.GetStatusCode(), outcome);
	if (loginCode == nullptr)
	{
		return outcome;
	}

	return Succeed(response.GetStatusCode(),
				   {{"code", StringOrNull(*loginCode, "code")}, {"expiresAt", StringOrNull(*loginCode, "expiresAt")}});
}

CallOutcome ParseUserModulesResponse(const HttpResponse& response)
{
	CallOutcome outcome;
	Json responseBody;
	if (!CheckResponse(response, responseBody, outcome))
	{
		return outcome;
	}

	const Json* modulesByUser = FindMemberIgnoringCase(responseBody, "Data");
	if (modulesByUser == nullptr || !modulesByUser->is_object())
	{
		return RejectResponse(responseBody, response.GetStatusCode(), "User modules response had no data");
	}

	Json userModules = Json::array();
	for (const auto& [userIdText, sourceModuleIds] : modulesByUser->items())
	{
		Json moduleIds = Json::array();
		if (sourceModuleIds.is_array())
		{
			for (const Json& sourceModuleId : sourceModuleIds)
			{
				const std::optional<int64_t> moduleId = ConvertToInteger(sourceModuleId);
				if (moduleId)
				{
					moduleIds.push_back(*moduleId);
				}
			}
		}

		const std::optional<int64_t> userId = ConvertToInteger(Json(userIdText));
		userModules.push_back({{"userId", userId ? Json(*userId) : Json(userIdText)}, {"moduleIds", moduleIds}});
	}

	return Succeed(response.GetStatusCode(), {{"users", userModules}});
}

CallOutcome ParseModuleAccessResponse(const HttpResponse& response)
{
	CallOutcome outcome;
	Json responseBody;
	if (!CheckResponse(response, responseBody, outcome))
	{
		return outcome;
	}

	const std::optional<int64_t> userId = GetInteger(responseBody, "UserId");
	const std::optional<int64_t> moduleId = GetInteger(responseBody, "ModuleId");
	if (!userId || !moduleId)
	{
		return RejectResponse(responseBody, response.GetStatusCode(), "Module access response had no user or module");
	}

	return Succeed(response.GetStatusCode(), {
												 {"userId", *userId},
												 {"moduleId", *moduleId},
												 {"access", GetBoolean(responseBody, "Access").value_or(false)},
												 {"passingScore", NumberOrNull(responseBody, "PassingScore")},
											 });
}

CallOutcome ParseModuleCatalogResponse(const HttpResponse& response)
{
	CallOutcome outcome;
	Json responseBody;
	if (!CheckResponse(response, responseBody, outcome))
	{
		return outcome;
	}

	const Json* graphQlUser = FindGraphQlData(responseBody, "user", response.GetStatusCode(), outcome);
	if (graphQlUser == nullptr)
	{
		return outcome;
	}

	const Json* sourceModules = FindArray(*graphQlUser, "modules");
	return Succeed(response.GetStatusCode(), {{"modules", sourceModules != nullptr ? *sourceModules : Json::array()}});
}

Json FilterModules(const Json& catalogModules, const std::string& platformName, uint32_t lifecycleMask)
{
	Json normalizedModules = Json::array();
	if (!catalogModules.is_array())
	{
		return normalizedModules;
	}

	for (const Json& sourceModule : catalogModules)
	{
		Json normalizedModule = NormalizeModule(sourceModule, platformName, lifecycleMask);
		// A module with nothing to launch here is not offered.
		if (normalizedModule["versions"].empty())
		{
			continue;
		}

		normalizedModules.push_back(std::move(normalizedModule));
	}

	return normalizedModules;
}

void SortModules(Json& normalizedModules, EApexModuleSort sortOrder)
{
	if (!normalizedModules.is_array() || sortOrder == APEX_MODULE_SORT_PLATFORM)
	{
		return;
	}

	// Sort keys are computed once; ties keep the platform's order.
	std::vector<std::string> textKeys(normalizedModules.size());
	std::vector<int64_t> numberKeys(normalizedModules.size());
	std::vector<size_t> sortedIndices(normalizedModules.size());
	for (size_t moduleIndex = 0; moduleIndex < normalizedModules.size(); ++moduleIndex)
	{
		const Json& normalizedModule = normalizedModules[moduleIndex];
		sortedIndices[moduleIndex] = moduleIndex;
		if (sortOrder == APEX_MODULE_SORT_NAME)
		{
			textKeys[moduleIndex] = ToLowerText(GetString(normalizedModule, "name").value_or(""));
		}
		else if (sortOrder == APEX_MODULE_SORT_ID)
		{
			numberKeys[moduleIndex] = GetInteger(normalizedModule, "id").value_or(-1);
		}
		else
		{
			textKeys[moduleIndex] = GetNewestVersionTime(normalizedModule);
		}
	}

	std::stable_sort(sortedIndices.begin(), sortedIndices.end(),
					 [sortOrder, &textKeys, &numberKeys](size_t firstIndex, size_t secondIndex)
	{
		if (sortOrder == APEX_MODULE_SORT_ID)
		{
			return numberKeys[firstIndex] < numberKeys[secondIndex];
		}

		const std::string& firstKey = textKeys[firstIndex];
		const std::string& secondKey = textKeys[secondIndex];
		// Modules without a name or a version time go last.
		if (firstKey.empty() != secondKey.empty())
		{
			return secondKey.empty();
		}

		return sortOrder == APEX_MODULE_SORT_NAME ? firstKey < secondKey : firstKey > secondKey;
	});

	Json sortedModules = Json::array();
	for (const size_t moduleIndex : sortedIndices)
	{
		sortedModules.push_back(std::move(normalizedModules[moduleIndex]));
	}

	normalizedModules = std::move(sortedModules);
}

Json BuildModulePage(const Json& normalizedModules, int32_t pageIndex, int32_t pageSize)
{
	const size_t totalCount = normalizedModules.is_array() ? normalizedModules.size() : 0;
	const size_t pageCount = pageSize > 0 ? (totalCount + static_cast<size_t>(pageSize) - 1) / pageSize : 0;
	Json pageModules = Json::array();
	if (pageIndex >= 0 && pageSize > 0)
	{
		const size_t firstIndex = static_cast<size_t>(pageIndex) * static_cast<size_t>(pageSize);
		const size_t endIndex = std::min(totalCount, firstIndex + static_cast<size_t>(pageSize));
		for (size_t moduleIndex = firstIndex; moduleIndex < endIndex; ++moduleIndex)
		{
			pageModules.push_back(normalizedModules[moduleIndex]);
		}
	}

	return {
		{"modules", pageModules}, {"pageIndex", pageIndex},   {"pageSize", pageSize},
		{"pageCount", pageCount}, {"totalCount", totalCount},
	};
}

CallOutcome ParseModuleListResponse(const HttpResponse& response, const std::string& platformName)
{
	CallOutcome outcome = ParseModuleCatalogResponse(response);
	if (outcome.Succeeded())
	{
		outcome.SetPayload(
			{{"modules", FilterModules(outcome.GetPayload()["modules"], platformName, kEveryReturnedLifecycle)}});
	}

	return outcome;
}

CallOutcome ParseSessionJoinResponse(const HttpResponse& response)
{
	CallOutcome outcome;
	Json responseBody;
	if (!CheckResponse(response, responseBody, outcome))
	{
		return outcome;
	}

	std::optional<int64_t> sessionId = GetInteger(responseBody, "sessionId");
	if (!sessionId)
	{
		// The legacy /event response nests it: {"Data": {"SessionId": n}}.
		const Json* legacyData = FindMemberIgnoringCase(responseBody, "Data");
		if (legacyData != nullptr)
		{
			sessionId = GetInteger(*legacyData, "SessionId");
		}
	}

	if (!sessionId || *sessionId <= 0)
	{
		return RejectResponse(responseBody, response.GetStatusCode(), "Join response had no session ID");
	}

	return Succeed(response.GetStatusCode(), {{"sessionId", *sessionId}});
}

CallOutcome ParseAcknowledgementResponse(const HttpResponse& response)
{
	CallOutcome outcome;
	Json responseBody;
	CheckResponse(response, responseBody, outcome);
	return outcome;
}

}  // namespace endpoints
}  // namespace apex
