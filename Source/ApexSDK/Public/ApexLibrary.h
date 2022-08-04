// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VaRestJsonValue.h"
#include "Dom/JsonValue.h"
#include "ApexJSON.h"
#include "XAPIExtension.h"
#include "ApexLibrary.generated.h"

/**
 * 
 */
UCLASS()
class APEXSDK_API UApexLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Apex|API")
	static FXAPIExtension CreateExtensionData(const TMap<FString, UVaRestJsonValue*>& Data);
	
public:
	static TArray<TSharedPtr<FJsonValue>> ConvertToJsonValueStringArray(const TArray<FString>& Array)
	{
		TArray<TSharedPtr<FJsonValue>> ConvertedArray;
		for (const FString& Item : Array)
		{
			TSharedPtr<FJsonValueString> ItemPtr(new FJsonValueString(Item));
			ConvertedArray.Add(ItemPtr);
		}

		return ConvertedArray;
	}

	static TArray<TSharedPtr<FJsonValue>> ConvertToJsonValueObjectArray(const TArray<FJsonable>& Array)
	{
		TArray<TSharedPtr<FJsonValue>> ConvertedArray;
		for (const FJsonable& Item : Array)
		{
			TSharedPtr<FJsonValueObject> ItemPtr(new FJsonValueObject(Item.ToJsonObject()));
			ConvertedArray.Add(ItemPtr);
		}

		return ConvertedArray;
	}
};
