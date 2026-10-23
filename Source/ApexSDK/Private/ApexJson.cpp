// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#include "ApexJson.h"

#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace ApexJson
{
TSharedPtr<FJsonObject> ParseObject(const FString& jsonText)
{
	TSharedPtr<FJsonObject> jsonObject;
	const TSharedRef<TJsonReader<>> jsonReader = TJsonReaderFactory<>::Create(jsonText);
	if (jsonText.IsEmpty() || !FJsonSerializer::Deserialize(jsonReader, jsonObject))
	{
		return nullptr;
	}

	return jsonObject;
}

FString Write(const TSharedRef<FJsonObject>& jsonObject)
{
	FString jsonText;
	const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> jsonWriter =
		TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&jsonText);
	FJsonSerializer::Serialize(jsonObject, jsonWriter);
	return jsonText;
}

FString GetString(const TSharedPtr<FJsonObject>& jsonObject, const FString& fieldName)
{
	FString fieldText;
	if (jsonObject.IsValid() && jsonObject->TryGetStringField(fieldName, fieldText))
	{
		return fieldText;
	}

	return FString();
}

int64 GetInteger(const TSharedPtr<FJsonObject>& jsonObject, const FString& fieldName)
{
	return static_cast<int64>(GetNumber(jsonObject, fieldName));
}

double GetNumber(const TSharedPtr<FJsonObject>& jsonObject, const FString& fieldName, bool* hasNumber)
{
	double fieldNumber = 0.0;
	const bool foundNumber = jsonObject.IsValid() && jsonObject->TryGetNumberField(fieldName, fieldNumber);
	if (hasNumber != nullptr)
	{
		*hasNumber = foundNumber;
	}

	return foundNumber ? fieldNumber : 0.0;
}

bool GetBoolean(const TSharedPtr<FJsonObject>& jsonObject, const FString& fieldName)
{
	bool fieldValue = false;
	return jsonObject.IsValid() && jsonObject->TryGetBoolField(fieldName, fieldValue) && fieldValue;
}

TSharedPtr<FJsonObject> GetObject(const TSharedPtr<FJsonObject>& jsonObject, const FString& fieldName)
{
	const TSharedPtr<FJsonObject>* fieldObject = nullptr;
	if (jsonObject.IsValid() && jsonObject->TryGetObjectField(fieldName, fieldObject) && fieldObject != nullptr)
	{
		return *fieldObject;
	}

	return nullptr;
}

static TArray<TSharedPtr<FJsonValue>> GetArray(const TSharedPtr<FJsonObject>& jsonObject, const FString& fieldName)
{
	const TArray<TSharedPtr<FJsonValue>>* fieldArray = nullptr;
	if (jsonObject.IsValid() && jsonObject->TryGetArrayField(fieldName, fieldArray) && fieldArray != nullptr)
	{
		return *fieldArray;
	}

	return TArray<TSharedPtr<FJsonValue>>();
}

static TArray<TSharedPtr<FJsonObject>> GetObjectArray(const TSharedPtr<FJsonObject>& jsonObject,
													  const FString& fieldName)
{
	TArray<TSharedPtr<FJsonObject>> objects;
	for (const TSharedPtr<FJsonValue>& arrayValue : GetArray(jsonObject, fieldName))
	{
		const TSharedPtr<FJsonObject>* elementObject = nullptr;
		if (arrayValue.IsValid() && arrayValue->TryGetObject(elementObject) && elementObject != nullptr)
		{
			objects.Add(*elementObject);
		}
	}

	return objects;
}

TArray<FString> GetStringArray(const TSharedPtr<FJsonObject>& jsonObject, const FString& fieldName)
{
	TArray<FString> strings;
	for (const TSharedPtr<FJsonValue>& arrayValue : GetArray(jsonObject, fieldName))
	{
		FString elementText;
		if (arrayValue.IsValid() && arrayValue->TryGetString(elementText))
		{
			strings.Add(elementText);
		}
	}

	return strings;
}

static EApexModuleLifecycle ParseLifecycleStage(const FString& stageName)
{
	if (stageName == TEXT("RELEASED"))
	{
		return EApexModuleLifecycle::RELEASED;
	}

	if (stageName == TEXT("DEVELOPMENT"))
	{
		return EApexModuleLifecycle::DEVELOPMENT;
	}

	if (stageName == TEXT("QA"))
	{
		return EApexModuleLifecycle::QA;
	}

	if (stageName == TEXT("UAT"))
	{
		return EApexModuleLifecycle::UAT;
	}

	return EApexModuleLifecycle::OTHER;
}

FApexUser ParseUser(const FString& payloadJson)
{
	const TSharedPtr<FJsonObject> userObject = ParseObject(payloadJson);
	FApexUser user;
	user.UserId = static_cast<int32>(GetInteger(userObject, TEXT("id")));
	user.OrganizationId = static_cast<int32>(GetInteger(userObject, TEXT("orgId")));
	user.FirstName = GetString(userObject, TEXT("firstName"));
	user.LastName = GetString(userObject, TEXT("lastName"));
	user.Username = GetString(userObject, TEXT("username"));
	user.Email = GetString(userObject, TEXT("email"));
	user.Role = GetString(userObject, TEXT("role"));
	user.Token = GetString(userObject, TEXT("token"));
	user.MinimumPassingScore = static_cast<float>(GetNumber(userObject, TEXT("minimumPassingScore")));

	const TSharedPtr<FJsonObject> organizationObject = GetObject(userObject, TEXT("org"));
	user.Organization.OrganizationId = static_cast<int32>(GetInteger(organizationObject, TEXT("id")));
	user.Organization.Name = GetString(organizationObject, TEXT("name"));
	user.Organization.Status = GetString(organizationObject, TEXT("status"));
	user.Organization.DownloadRegion = GetString(organizationObject, TEXT("downloadRegion"));
	user.Organization.Type = GetString(organizationObject, TEXT("type"));
	user.Organization.HubLogoLink = GetString(organizationObject, TEXT("hubLogoLink"));
	user.Organization.PrimaryColor = GetString(organizationObject, TEXT("primaryColor"));
	user.Organization.SecondaryColor = GetString(organizationObject, TEXT("secondaryColor"));
	return user;
}

FApexModuleAccess ParseModuleAccess(const FString& payloadJson)
{
	const TSharedPtr<FJsonObject> accessObject = ParseObject(payloadJson);
	FApexModuleAccess moduleAccess;
	moduleAccess.UserId = static_cast<int32>(GetInteger(accessObject, TEXT("userId")));
	moduleAccess.ModuleId = static_cast<int32>(GetInteger(accessObject, TEXT("moduleId")));
	moduleAccess.bHasAccess = GetBoolean(accessObject, TEXT("access"));
	moduleAccess.PassingScore =
		static_cast<float>(GetNumber(accessObject, TEXT("passingScore"), &moduleAccess.bHasPassingScore));
	return moduleAccess;
}

FApexQuickIdUsers ParseQuickIdUsers(const FString& payloadJson)
{
	const TSharedPtr<FJsonObject> payloadObject = ParseObject(payloadJson);
	const TSharedPtr<FJsonObject> organizationObject = GetObject(payloadObject, TEXT("org"));
	FApexQuickIdUsers quickIdUsers;
	quickIdUsers.OrganizationName = GetString(organizationObject, TEXT("name"));
	quickIdUsers.PrimaryColor = GetString(organizationObject, TEXT("primaryColor"));
	quickIdUsers.SecondaryColor = GetString(organizationObject, TEXT("secondaryColor"));
	quickIdUsers.HubLogoUrl = GetString(organizationObject, TEXT("hubLogoUrl"));
	for (const TSharedPtr<FJsonObject>& userObject : GetObjectArray(payloadObject, TEXT("users")))
	{
		FApexQuickIdUser& quickIdUser = quickIdUsers.Users.AddDefaulted_GetRef();
		quickIdUser.FirstName = GetString(userObject, TEXT("firstName"));
		quickIdUser.LastName = GetString(userObject, TEXT("lastName"));
		quickIdUser.Username = GetString(userObject, TEXT("username"));
		quickIdUser.Email = GetString(userObject, TEXT("email"));
	}

	return quickIdUsers;
}

FApexLoginCode ParseLoginCode(const FString& payloadJson)
{
	const TSharedPtr<FJsonObject> codeObject = ParseObject(payloadJson);
	FApexLoginCode loginCode;
	loginCode.Code = GetString(codeObject, TEXT("code"));
	loginCode.ExpiresAt = GetString(codeObject, TEXT("expiresAt"));
	return loginCode;
}

TArray<FApexUserModules> ParseUserModules(const FString& payloadJson)
{
	TArray<FApexUserModules> usersModules;
	for (const TSharedPtr<FJsonObject>& userObject : GetObjectArray(ParseObject(payloadJson), TEXT("users")))
	{
		FApexUserModules& userModules = usersModules.AddDefaulted_GetRef();
		userModules.UserId = static_cast<int32>(GetInteger(userObject, TEXT("userId")));
		for (const TSharedPtr<FJsonValue>& moduleIdValue : GetArray(userObject, TEXT("moduleIds")))
		{
			double moduleIdNumber = 0.0;
			if (moduleIdValue.IsValid() && moduleIdValue->TryGetNumber(moduleIdNumber))
			{
				userModules.ModuleIds.Add(static_cast<int32>(moduleIdNumber));
			}
		}
	}

	return usersModules;
}

static FApexModule ParseModule(const TSharedPtr<FJsonObject>& moduleObject)
{
	FApexModule module;
	module.ModuleId = static_cast<int32>(GetInteger(moduleObject, TEXT("id")));
	module.Name = GetString(moduleObject, TEXT("name"));
	module.Abbreviation = GetString(moduleObject, TEXT("abbreviation"));
	module.ExternalId = GetString(moduleObject, TEXT("externalId"));
	module.IconUrl = GetString(moduleObject, TEXT("iconUrl"));
	module.Distributor = GetString(moduleObject, TEXT("distributor"));
	module.ShortDescription = GetString(moduleObject, TEXT("shortDescription"));
	module.LongDescription = GetString(moduleObject, TEXT("longDescription"));
	module.Industry = GetString(moduleObject, TEXT("industry"));
	module.Details = GetString(moduleObject, TEXT("details"));
	module.Categories = GetString(moduleObject, TEXT("categories"));
	module.bIsAvailable = GetBoolean(moduleObject, TEXT("isAvailable"));
	module.bIsAuthenticatedLaunch = GetBoolean(moduleObject, TEXT("isAuthenticatedLaunch"));
	module.Languages = GetStringArray(moduleObject, TEXT("languages"));

	const TSharedPtr<FJsonObject> playerObject = GetObject(moduleObject, TEXT("player"));
	module.Player.bIsSet = playerObject.IsValid();
	module.Player.PlayerId = static_cast<int32>(GetInteger(playerObject, TEXT("id")));
	module.Player.Name = GetString(playerObject, TEXT("name"));
	module.Player.LaunchProtocol = GetString(playerObject, TEXT("launchProtocol"));
	for (const TSharedPtr<FJsonObject>& playerVersionObject : GetObjectArray(playerObject, TEXT("versions")))
	{
		FApexModulePlayerVersion& playerVersion = module.Player.Versions.AddDefaulted_GetRef();
		playerVersion.VersionId = static_cast<int32>(GetInteger(playerVersionObject, TEXT("id")));
		playerVersion.Version = GetString(playerVersionObject, TEXT("version"));
		playerVersion.Status = GetString(playerVersionObject, TEXT("status"));
		playerVersion.Package = GetString(playerVersionObject, TEXT("package"));
		playerVersion.Platforms = GetStringArray(playerVersionObject, TEXT("platforms"));
	}

	for (const TSharedPtr<FJsonObject>& versionObject : GetObjectArray(moduleObject, TEXT("versions")))
	{
		FApexModuleVersion& moduleVersion = module.Versions.AddDefaulted_GetRef();
		moduleVersion.VersionId = static_cast<int32>(GetInteger(versionObject, TEXT("id")));
		moduleVersion.Version = GetString(versionObject, TEXT("version"));
		moduleVersion.Package = GetString(versionObject, TEXT("package"));
		moduleVersion.CreatedAt = GetString(versionObject, TEXT("createdAt"));
		moduleVersion.Controls = GetStringArray(versionObject, TEXT("controls"));
		moduleVersion.Platforms = GetStringArray(versionObject, TEXT("platforms"));
		const TSharedPtr<FJsonObject> lifecycleObject = GetObject(versionObject, TEXT("lifecycle"));
		moduleVersion.Lifecycle.LifecycleId = static_cast<int32>(GetInteger(lifecycleObject, TEXT("id")));
		moduleVersion.Lifecycle.Name = GetString(lifecycleObject, TEXT("name"));
		moduleVersion.Lifecycle.Stage = ParseLifecycleStage(GetString(lifecycleObject, TEXT("stage")));
	}

	return module;
}

TArray<FApexModule> ParseModuleList(const FString& payloadJson)
{
	TArray<FApexModule> modules;
	for (const TSharedPtr<FJsonObject>& moduleObject : GetObjectArray(ParseObject(payloadJson), TEXT("modules")))
	{
		modules.Add(ParseModule(moduleObject));
	}

	return modules;
}

FApexModulePage ParseModulePage(const FString& payloadJson)
{
	const TSharedPtr<FJsonObject> pageObject = ParseObject(payloadJson);
	FApexModulePage modulePage;
	for (const TSharedPtr<FJsonObject>& moduleObject : GetObjectArray(pageObject, TEXT("modules")))
	{
		modulePage.Modules.Add(ParseModule(moduleObject));
	}

	modulePage.PageIndex = static_cast<int32>(GetInteger(pageObject, TEXT("pageIndex")));
	modulePage.PageSize = static_cast<int32>(GetInteger(pageObject, TEXT("pageSize")));
	modulePage.PageCount = static_cast<int32>(GetInteger(pageObject, TEXT("pageCount")));
	modulePage.TotalCount = static_cast<int32>(GetInteger(pageObject, TEXT("totalCount")));
	return modulePage;
}

int64 ParseSessionId(const FString& payloadJson)
{
	return GetInteger(ParseObject(payloadJson), TEXT("sessionId"));
}
}  // namespace ApexJson
