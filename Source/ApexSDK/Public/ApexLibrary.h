// Copyright 2022 PixoVR Corp. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VaRestJsonValue.h"
#include "Dom/JsonValue.h"
#include "ApexJSON.h"
#include "XAPIExtension.h"
#include "VaRestSubsystem.h"
#include "ApexLibrary.generated.h"

/**
 * 
 */
UCLASS()
class APEXSDK_API UApexLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public: // Data related type functions

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

	static bool TryFillFromStringField(const TSharedPtr<FJsonObject>& JObject, const FString Field, FString& ValueToFill)
	{
		if (JObject->HasField(Field))
		{
			ValueToFill = JObject->GetStringField(Field);
			return true;
		}

		return false;
	}

	static bool TryFillFromBoolField(const TSharedPtr<FJsonObject>& JObject, const FString Field, bool& ValueToFill)
	{
		if (JObject->HasField(Field))
		{
			ValueToFill = JObject->GetBoolField(Field);
			return true;
		}

		return false;
	}

	static bool TryFillFromNumberField(const TSharedPtr<FJsonObject>& JObject, const FString Field, float& ValueToFill)
	{
		if (JObject->HasField(Field))
		{
			ValueToFill = JObject->GetNumberField(Field);
			return true;
		}

		return false;
	}

	static bool TryFillFromIntegerField(const TSharedPtr<FJsonObject>& JObject, const FString Field, int& ValueToFill)
	{
		if (JObject->HasField(Field))
		{
			ValueToFill = JObject->GetIntegerField(Field);
			return true;
		}

		return false;
	}

	static bool TryFillFromObjectField(const TSharedPtr<FJsonObject>& JObject, const FString Field, FJsonable& ValueToFill)
	{
		if (JObject->HasField(Field))
		{
			ValueToFill.FromJsonObject(JObject->GetObjectField(Field));
			return true;
		}

		return false;
	}

	static bool TryFillFromObjectArrayField(const TSharedPtr<FJsonObject>& JObject, const FString Field, TArray<FJsonable>& ArrayToFill)
	{
		if (JObject->HasField(Field))
		{
			TArray<TSharedPtr<FJsonValue>> Array = JObject->GetArrayField(Field);
			FJsonable Jsonable;
			for (TSharedPtr<FJsonValue> Value : Array)
			{
				Jsonable.FromJsonObject(Value->AsObject());
				ArrayToFill.Add(Jsonable);
			}
			return true;
		}

		return false;
	}

	static bool TryFillFromStringArrayField(const TSharedPtr<FJsonObject>& JObject, const FString Field, TArray<FString>& ArrayToFill)
	{
		if (JObject->HasField(Field))
		{
			TArray<TSharedPtr<FJsonValue>> Array = JObject->GetArrayField(Field);
			for (TSharedPtr<FJsonValue> Value : Array)
			{
				ArrayToFill.Add(Value->AsString());
			}
			return true;
		}

		return false;
	}

	static bool TryFillFromNumberArrayField(const TSharedPtr<FJsonObject>& JObject, const FString Field, TArray<float>& ArrayToFill)
	{
		if (JObject->HasField(Field))
		{
			TArray<TSharedPtr<FJsonValue>> Array = JObject->GetArrayField(Field);
			for (TSharedPtr<FJsonValue> Value : Array)
			{
				ArrayToFill.Add(Value->AsNumber());
			}
			return true;
		}

		return false;
	}

	static bool TryFillFromIntegerArrayField(const TSharedPtr<FJsonObject>& JObject, const FString Field, TArray<int>& ArrayToFill)
	{
		if (JObject->HasField(Field))
		{
			TArray<TSharedPtr<FJsonValue>> Array = JObject->GetArrayField(Field);
			for (TSharedPtr<FJsonValue> Value : Array)
			{
				ArrayToFill.Add(Value->AsNumber());
			}
			return true;
		}

		return false;
	}

	static bool TryFillFromBoolArrayField(const TSharedPtr<FJsonObject>& JObject, const FString Field, TArray<bool>& ArrayToFill)
	{
		if (JObject->HasField(Field))
		{
			TArray<TSharedPtr<FJsonValue>> Array = JObject->GetArrayField(Field);
			for (TSharedPtr<FJsonValue> Value : Array)
			{
				ArrayToFill.Add(Value->AsBool());
			}
			return true;
		}

		return false;
	}

	static UVaRestSubsystem* GetRestSubsystem()
	{
		static UVaRestSubsystem* VaRestSubsystem = GEngine->GetEngineSubsystem<UVaRestSubsystem>();
		return VaRestSubsystem;
	}

	static FString GetApexSDKVersion();
};