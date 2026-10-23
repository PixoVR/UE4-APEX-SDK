// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ApexBlueprintLibrary.generated.h"

class UApexSubsystem;

/** Helpers for building the JSON the session calls take, and other small things. */
UCLASS()
class APEXSDK_API UApexBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** The Apex subsystem of this world's game instance. */
	UFUNCTION(BlueprintPure, Category = "Apex", meta = (WorldContext = "worldContextObject", DisplayName = "Get Apex"))
	static UApexSubsystem* GetApex(const UObject* worldContextObject);

	/** A JSON object with string values, e.g. for session or event data: {"key": "value", ...}. */
	UFUNCTION(BlueprintPure, Category = "Apex|JSON")
	static FString MakeJsonObject(const TMap<FString, FString>& stringFields);

	/** Adds a number field to a JSON object (empty text starts a new object); returns the new text. */
	UFUNCTION(BlueprintPure, Category = "Apex|JSON")
	static FString AddJsonNumber(const FString& jsonObjectText, const FString& fieldName, float fieldNumber);

	/** Adds a string field to a JSON object (empty text starts a new object); returns the new text. */
	UFUNCTION(BlueprintPure, Category = "Apex|JSON")
	static FString AddJsonString(const FString& jsonObjectText, const FString& fieldName, const FString& fieldText);

	/** Adds a boolean field to a JSON object (empty text starts a new object); returns the new text. */
	UFUNCTION(BlueprintPure, Category = "Apex|JSON")
	static FString AddJsonBoolean(const FString& jsonObjectText, const FString& fieldName, bool fieldValue);

	/** Adds a nested JSON object to a JSON object (empty text starts a new object); returns the new text. */
	UFUNCTION(BlueprintPure, Category = "Apex|JSON")
	static FString AddJsonObject(const FString& jsonObjectText, const FString& fieldName,
								 const FString& fieldObjectText);

	/** True for MAJOR.MINOR.PATCH with a major that does not start with 0 and 1-2 digit minor and patch. */
	UFUNCTION(BlueprintPure, Category = "Apex")
	static bool IsModuleVersionValid(const FString& moduleVersion);
};
