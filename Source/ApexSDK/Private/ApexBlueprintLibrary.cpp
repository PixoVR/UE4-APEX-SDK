// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#include "ApexBlueprintLibrary.h"

#include "ApexJson.h"
#include "ApexSubsystem.h"

namespace
{
TSharedRef<FJsonObject> ParseOrCreate(const FString& jsonObjectText)
{
	const TSharedPtr<FJsonObject> parsedObject = ApexJson::ParseObject(jsonObjectText);
	return parsedObject.IsValid() ? parsedObject.ToSharedRef() : MakeShared<FJsonObject>();
}
}  // namespace

UApexSubsystem* UApexBlueprintLibrary::GetApex(const UObject* worldContextObject)
{
	return UApexSubsystem::Get(worldContextObject);
}

FString UApexBlueprintLibrary::MakeJsonObject(const TMap<FString, FString>& stringFields)
{
	TSharedRef<FJsonObject> jsonObject = MakeShared<FJsonObject>();
	for (const TPair<FString, FString>& stringField : stringFields)
	{
		jsonObject->SetStringField(stringField.Key, stringField.Value);
	}

	return ApexJson::Write(jsonObject);
}

FString UApexBlueprintLibrary::AddJsonNumber(const FString& jsonObjectText, const FString& fieldName, float fieldNumber)
{
	TSharedRef<FJsonObject> jsonObject = ParseOrCreate(jsonObjectText);
	jsonObject->SetNumberField(fieldName, fieldNumber);
	return ApexJson::Write(jsonObject);
}

FString UApexBlueprintLibrary::AddJsonString(const FString& jsonObjectText, const FString& fieldName,
											 const FString& fieldText)
{
	TSharedRef<FJsonObject> jsonObject = ParseOrCreate(jsonObjectText);
	jsonObject->SetStringField(fieldName, fieldText);
	return ApexJson::Write(jsonObject);
}

FString UApexBlueprintLibrary::AddJsonBoolean(const FString& jsonObjectText, const FString& fieldName, bool fieldValue)
{
	TSharedRef<FJsonObject> jsonObject = ParseOrCreate(jsonObjectText);
	jsonObject->SetBoolField(fieldName, fieldValue);
	return ApexJson::Write(jsonObject);
}

FString UApexBlueprintLibrary::AddJsonObject(const FString& jsonObjectText, const FString& fieldName,
											 const FString& fieldObjectText)
{
	TSharedRef<FJsonObject> jsonObject = ParseOrCreate(jsonObjectText);
	jsonObject->SetObjectField(fieldName, ParseOrCreate(fieldObjectText));
	return ApexJson::Write(jsonObject);
}

bool UApexBlueprintLibrary::IsModuleVersionValid(const FString& moduleVersion)
{
	return ApexModuleVersionIsValid(TCHAR_TO_UTF8(*moduleVersion)) != 0;
}
