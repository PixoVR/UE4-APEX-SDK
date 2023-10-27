// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"
#include "Dom/JsonValue.h"
#include "XAPIBase.h"
#include "XAPIExtension.generated.h"

USTRUCT(BlueprintType)
struct PIXODATAREPORTING_API FXAPIExtension : public FXAPIBase
{
	GENERATED_BODY()

	TMap<FString, TSharedPtr<FJsonValue>> Fields;

public:

	void Clone(const FXAPIExtension& InExtension)
	{
		Fields.Empty();
		Fields.Append(InExtension.Fields);
	}

	FXAPIExtension() {}

	virtual ~FXAPIExtension() {}

	FXAPIExtension(const FXAPIExtension& InExtension)
	{
		Fields.Append(InExtension.Fields);
	}

	FXAPIExtension& operator=(const FXAPIExtension& InExtension)
	{
		Clone(InExtension);
		return *this;
	}

	// String
	TSharedPtr<FJsonValue>& AddString(FString InKey, const FString& InValue)
	{
		TSharedPtr<FJsonValueString> ValuePtr(new FJsonValueString(InValue));
		return Add(InKey, ValuePtr);
	}

	TSharedPtr<FJsonValue>& AddCustomString(FString InKey, const FString& InValue)
	{
		TSharedPtr<FJsonValueString> ValuePtr(new FJsonValueString(InValue));
		return AddCustom(InKey, ValuePtr);
	}

	// Numbers
	TSharedPtr<FJsonValue>& AddNumber(FString InKey, const int& InValue)
	{
		TSharedPtr<FJsonValue> ValuePtr(new FJsonValueNumber(InValue));
		return Add(InKey, ValuePtr);
	}

	TSharedPtr<FJsonValue>& AddCustomNumber(FString InKey, const int& InValue)
	{
		TSharedPtr<FJsonValue> ValuePtr(new FJsonValueNumber(InValue));
		return AddCustom(InKey, ValuePtr);
	}

	// Boolean
	TSharedPtr<FJsonValue>& AddBoolean(FString InKey, const bool& InValue)
	{
		TSharedPtr<FJsonValue> ValuePtr(new FJsonValueBoolean(InValue));
		return Add(InKey, ValuePtr);
	}

	TSharedPtr<FJsonValue>& AddCustomBoolean(FString InKey, const bool& InValue)
	{
		TSharedPtr<FJsonValue> ValuePtr(new FJsonValueBoolean(InValue));
		return AddCustom(InKey, ValuePtr);
	}

	// Custom
	TSharedPtr<FJsonValue>& Add(FString InKey, const TSharedPtr<FJsonValue>& InValue)
	{
		return Fields.Add("https://pixovr.com/xapi/extension/" + InKey, InValue);
	}

	TSharedPtr<FJsonValue>& AddCustom(FString InKey, const TSharedPtr<FJsonValue>& InValue)
	{
		return Fields.Add(InKey, InValue);
	}

	virtual bool IsEmpty() const override
	{
		return Fields.Num() == 0;
	}

	virtual TSharedPtr<FJsonObject> ToJsonObject() const override
	{
		TSharedPtr<FJsonObject> Result(new FJsonObject);

		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Fields)
		{
			Result->SetField(Pair.Key, Pair.Value);
		}

		return Result;
	}
};