// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ApexSDKDefines.h"
#include "XAPITypes.h"

#include "PixoConfigJSON.h"

#include "ApexTypes.generated.h"


USTRUCT(BlueprintType)
struct PIXODATAREPORTING_API FJoinSessionData : public FJsonable
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|Join Session")
	FString Uuid;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|Join Session")
	FString EventType;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|Join Session")
	int ModuleId = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|Join Session")
	FString DeviceId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|Join Session")
	FString IpAddress;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|Join Session")
	FXAPIStatement JsonData;

	FJoinSessionData() = default;

	virtual ~FJoinSessionData() {}

	virtual TSharedPtr<FJsonObject> ToJsonObject() const
	{
		TSharedPtr<FJsonObject> Result(new FJsonObject);
		if (!Uuid.IsEmpty())
		{
			Result->SetStringField("uuid", Uuid);
		}

		if (!EventType.IsEmpty())
		{
			Result->SetStringField("eventType", EventType);
		}

		Result->SetNumberField("moduleId", ModuleId);

		if (!DeviceId.IsEmpty())
		{
			Result->SetStringField("deviceId", DeviceId);
		}

		if (!IpAddress.IsEmpty())
		{
			Result->SetStringField("ipAddress", IpAddress);
		}

		JsonData.Stamp();

		if (!JsonData.IsEmpty())
		{
			Result->SetObjectField("jsonData", JsonData.ToJsonObject());
		}

		return Result;
	}
};

USTRUCT(BlueprintType)
struct PIXODATAREPORTING_API FSessionData : public FJsonable
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|Session Data")
	bool Complete;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|Session Data")
	bool Success;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|Session Data")
	float Score = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|Session Data")
	float ScoreScaled = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|Session Data")
	float ScoreMin = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|Session Data")
	float ScoreMax = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|Session Data")
	int Duration = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|Session Data")
	FXAPIExtension AdditionalContextData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|Session Data")
	FXAPIExtension AdditionalResultData;

	FSessionData() = default;

	virtual ~FSessionData() { }

	FSessionData() : Complete(false), Success(false), Score(0.0f), ScoreScaled(0.0f),
		ScoreMin(0.0f), ScoreMax(0.0f), Duration(0),
		AdditionalContextData(), AdditionalResultData() { }

	FSessionData(bool InComplete, bool InSuccess, float InScore, float InScoreScaled,
		float InScoreMin, float InScoreMax, int InDuration,
		FXAPIExtension InContextExtension, FXAPIExtension InResultExtension)
		: Complete(InComplete), Success(InSuccess), Score(InScore), ScoreScaled(InScoreScaled),
		ScoreMin(InScoreMin), ScoreMax(InScoreMax), Duration(InDuration),
		AdditionalContextData(InContextExtension), AdditionalResultData(InResultExtension)
	{ }

	// TODO: Implement a FromJsonObject function. IMPORTANT TO DO!
	virtual void FromJsonObject(const TSharedPtr<FJsonObject>& JObject) override
	{
		Score = JObject->GetNumberField("score");
		ScoreScaled = JObject->GetNumberField("scoreScaled");
		ScoreMin = JObject->GetNumberField("scoreMin");
		ScoreMax = JObject->GetNumberField("scoreMax");
		Duration = JObject->GetNumberField("sessionDuration");
		Complete = JObject->GetBoolField("success");
		FString LessonStatus = JObject->GetStringField("lessonStatus");

		if (LessonStatus.Equals("passed"))
		{
			Complete = true;
		}
		else
		{
			Complete = false;
		}
	}
};

USTRUCT(BlueprintType)
struct PIXODATAREPORTING_API FCompleteSessionData : public FJsonable
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|Complete Session")
	FString Uuid;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|Complete Session")
	FString EventType;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|Complete Session")
	int ModuleId = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|Complete Session")
	FString DeviceId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|Complete Session")
	FXAPIStatement JsonData;

	// TODO: This data will get packed into the FXAPIStatement instead of an extra layer
	FSessionData SessionData;

	FCompleteSessionData() = default;

	virtual ~FCompleteSessionData() { }

	// TODO: Implement a FromJsonObject function. IMPORTANT TO DO!
	virtual void FromJsonObject(const TSharedPtr<FJsonObject>& JObject) override
	{
		if (JObject->HasField("uuid"))
		{
			Uuid = JObject->GetStringField("uuid");
		}

		if (JObject->HasField("eventType"))
		{
			EventType = JObject->GetStringField("eventType");
		}

		ModuleId = JObject->GetNumberField("moduleId");

		if (JObject->HasField("deviceId"))
		{
			DeviceId = JObject->GetStringField("deviceId");
		}

		if (JObject->HasField("jsonData"))
		{
			JsonData.FromJsonObject(JObject->GetObjectField("jsonData"));
			SessionData.FromJsonObject(JObject->GetObjectField("jsonData"));
		}
	}

	virtual TSharedPtr<FJsonObject> ToJsonObject() const override
	{
		TSharedPtr<FJsonObject> Result(new FJsonObject);

		if (!Uuid.IsEmpty())
		{
			Result->SetStringField("uuid", Uuid);
		}

		if (!EventType.IsEmpty())
		{
			Result->SetStringField("eventType", EventType);
		}

		Result->SetNumberField("moduleId", ModuleId);

		if (!DeviceId.IsEmpty())
		{
			Result->SetStringField("deviceId", DeviceId);
		}

		// Update the time stamp
		JsonData.Stamp();

		if (!JsonData.IsEmpty())
		{
			TSharedPtr<FJsonObject> JsonDataObject = JsonData.ToJsonObject();
			JsonDataObject->SetNumberField("score", SessionData.Score);
			JsonDataObject->SetNumberField("scoreScaled", SessionData.ScoreScaled);
			JsonDataObject->SetNumberField("scoreMin", SessionData.ScoreMin);
			JsonDataObject->SetNumberField("scoreMax", SessionData.ScoreMax);
			JsonDataObject->SetNumberField("sessionDuration", SessionData.Duration);
			JsonDataObject->SetBoolField("success", SessionData.Success);
			FString LessonStatus = "failed";
			if (SessionData.Success)
			{
				LessonStatus = "passed";
			}
			JsonDataObject->SetStringField("lessonStatus", LessonStatus);
			JsonDataObject->SetStringField("moduleName", FString::FromInt(ModuleId));

			Result->SetObjectField("jsonData", JsonDataObject);
		}

		return Result;
	}
};

USTRUCT(BlueprintType)
struct PIXODATAREPORTING_API FSessionEventData : public FJsonable
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|Session Event")
	FString Uuid;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|Session Event")
	FString EventType;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|Session Event")
	int ModuleId = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|Session Event")
	FString DeviceId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|Session Event")
	FXAPIStatement JsonData;

public:
	FSessionEventData() = default;

	virtual ~FSessionEventData() { }

	virtual TSharedPtr<FJsonObject> ToJsonObject() const override
	{
		TSharedPtr<FJsonObject> Result(new FJsonObject);

		if (!Uuid.IsEmpty())
		{
			Result->SetStringField("uuid", Uuid);
		}

		if (!EventType.IsEmpty())
		{
			Result->SetStringField("eventType", EventType);
		}

		Result->SetNumberField("moduleId", ModuleId);

		if (!DeviceId.IsEmpty())
		{
			Result->SetStringField("deviceId", DeviceId);
		}

		// Update the time stamp
		JsonData.Stamp();

		if (!JsonData.IsEmpty())
		{
			Result->SetObjectField("jsonData", JsonData.ToJsonObject());
		}

		return Result;
	}
};

USTRUCT(BlueprintType)
struct PIXODATAREPORTING_API FApexUserData : public FJsonable
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|User Data")
	int Id = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|User Data")
	int OrgUnit = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|User Data")
	FString FirstName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|User Data")
	FString LastName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|User Data")
	FString Email;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|User Data")
	FString Username;

	FApexUserData() = default;

	virtual void FromJsonObject(const TSharedPtr<FJsonObject>& JObject) override
	{
		Id = JObject->GetIntegerField("id");
		if (JObject->HasField("orgunit"))
		{
			OrgUnit = JObject->GetIntegerField("orgunit");
		}
		else
		{
			OrgUnit = -1;
		}
		FirstName = JObject->GetStringField("first");
		LastName = JObject->GetStringField("last");
		Email = JObject->GetStringField("email");
		Username = JObject->GetStringField("username");
	}

	virtual TSharedPtr<FJsonObject> ToJsonObject() const override
	{
		TSharedPtr<FJsonObject> Result(new FJsonObject);

		Result->SetNumberField("id", Id);
		if (OrgUnit >= 0)
		{
			Result->SetNumberField("orgunit", OrgUnit);
		}
		Result->SetStringField("first", FirstName);
		Result->SetStringField("last", LastName);
		Result->SetStringField("email", Email);
		Result->SetStringField("username", Username);

		return Result;
	}
};

USTRUCT(BlueprintType)
struct PIXODATAREPORTING_API FSessionReportData : public FJsonable
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|Session Report")
	FString Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|Session Report")
	FString EventType;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|Session Report")
	int UserId = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|Session Report")
	int ModuleId = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|Session Report")
	int SessionId = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|Session Report")
	float SessionDuration = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|Session Report")
	bool LessonStatus;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|Session Report")
	FString CreatedAt;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|Session Report")
	FString UpdatedAt;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|Session Report")
	FXAPIStatement JsonData;

	FSessionReportData() = default;

	virtual ~FSessionReportData() { }

	virtual void FromJsonObject(const TSharedPtr<FJsonObject>& JObject) override
	{
		if (JObject->HasField("id"))
		{
			Id = JObject->GetStringField("id");
		}

		if (JObject->HasField("eventType"))
		{
			EventType = JObject->GetStringField("eventType");
		}

		if (JObject->HasField("sessionId"))
		{
			SessionId = JObject->GetNumberField("sessionId");
		}

		if (JObject->HasField("userId"))
		{
			UserId = JObject->GetNumberField("userId");
		}

		if (JObject->HasField("moduleId"))
		{
			ModuleId = JObject->GetNumberField("moduleId");
		}

		if (JObject->HasField("createdAt"))
		{
			CreatedAt = JObject->GetStringField("createdAt");
		}

		if (JObject->HasField("updatedAt"))
		{
			UpdatedAt = JObject->GetStringField("updatedAt");
		}

		if (JObject->HasField("jsonData"))
		{
			TSharedPtr<FJsonObject> JsonDataObject = JObject->GetObjectField("jsonData");
			JsonData.FromJsonObject(JsonDataObject);
			if (JsonDataObject->HasField("sessionDuration"))
			{
				SessionDuration = JsonDataObject->GetNumberField("sessionDuration");
			}

			if (JsonDataObject->HasField("lessonStatus"))
			{
				LessonStatus = JsonDataObject->GetBoolField("lessonStatus");
			}
		}
	}
};

USTRUCT(BlueprintType)
struct PIXODATAREPORTING_API FTopTenData : public FJsonable
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|Top Ten Data")
	bool Error;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|Top Ten Data")
	FString Message;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|Top Ten Data")
	FApexUserData User;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixo Platform|Top Ten Data")
	TArray<FSessionReportData> Sessions;

public:
	FTopTenData() = default;

	virtual ~FTopTenData() { }

	virtual void FromJsonObject(const TSharedPtr<FJsonObject>& JObject) override
	{
		Error = JObject->GetBoolField("error");
		Message = JObject->GetStringField("message");

		TArray<TSharedPtr<FJsonValue>> SessionDataArray;
		// This confirms if it's a user with session data or just session data
		if (JObject->HasTypedField<EJson::Array>("data"))
		{
			SessionDataArray = JObject->GetArrayField("data");
		}
		else
		{
			auto DataObject = JObject->GetObjectField("data");

			User.FromJsonObject(DataObject->GetObjectField("user"));
			SessionDataArray = DataObject->GetArrayField("sessions");
		}

		FSessionReportData CompleteSessionData;
		for (auto SessionData : SessionDataArray)
		{
			CompleteSessionData.FromJsonObject(SessionData->AsObject());

			Sessions.Push(CompleteSessionData);
		}
	}

	virtual TSharedPtr<FJsonObject> ToJsonObject() const override
	{
		TSharedPtr<FJsonObject> Result(new FJsonObject);

		//! \TODO (MGruber): Fill this in later. It's not super important right now.

		return Result;
	}
};

