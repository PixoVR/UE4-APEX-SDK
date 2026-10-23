// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ApexTypes.h"
#include "Dom/JsonObject.h"

/**
 * Turns the core's payloads (normalized JSON, spec/schema in apex-sdk-core) into Unreal types, and
 * builds the session JSON the SDK sends. Absent values arrive as null; they become empty strings,
 * zeros and empty arrays.
 */
namespace ApexJson
{
TSharedPtr<FJsonObject> ParseObject(const FString& jsonText);
FString Write(const TSharedRef<FJsonObject>& jsonObject);

FString GetString(const TSharedPtr<FJsonObject>& jsonObject, const FString& fieldName);
int64 GetInteger(const TSharedPtr<FJsonObject>& jsonObject, const FString& fieldName);
double GetNumber(const TSharedPtr<FJsonObject>& jsonObject, const FString& fieldName, bool* hasNumber = nullptr);
bool GetBoolean(const TSharedPtr<FJsonObject>& jsonObject, const FString& fieldName);
TSharedPtr<FJsonObject> GetObject(const TSharedPtr<FJsonObject>& jsonObject, const FString& fieldName);
TArray<FString> GetStringArray(const TSharedPtr<FJsonObject>& jsonObject, const FString& fieldName);

FApexUser ParseUser(const FString& payloadJson);
FApexModuleAccess ParseModuleAccess(const FString& payloadJson);
FApexQuickIdUsers ParseQuickIdUsers(const FString& payloadJson);
FApexLoginCode ParseLoginCode(const FString& payloadJson);
TArray<FApexUserModules> ParseUserModules(const FString& payloadJson);
TArray<FApexModule> ParseModuleList(const FString& payloadJson);
FApexModulePage ParseModulePage(const FString& payloadJson);
int64 ParseSessionId(const FString& payloadJson);
}  // namespace ApexJson
