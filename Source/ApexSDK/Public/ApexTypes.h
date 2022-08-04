// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ApexSDKDefines.h"
#include "XAPITypes.h"
#include "ApexJSON.h"
#include "ApexTypes.generated.h"

USTRUCT(BlueprintType)
struct APEXSDK_API FApexLoginInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Login")
		FString Username;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Login")
		FString Password;

public:

	FApexLoginInfo()
		: Username("")
		, Password("")
	{ }

	FApexLoginInfo(FString InUsername, FString InPassword)
		: Username(InUsername)
		, Password(InPassword)
	{ }
};

USTRUCT(BlueprintType)
struct APEXSDK_API FJoinSessionData : public FJsonable
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Join Session")
    FString Uuid;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Join Session")
    FString EventType;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Join Session")
    int ModuleID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Join Session")
    FString DeviceId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Join Session")
    FString IpAddress;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Join Session")
    FXAPIStatement JsonData;

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

        Result->SetNumberField("moduleId", ModuleID);

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
struct APEXSDK_API FSessionData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Data")
    bool Complete;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Data")
    bool Success;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Data")
    float Score;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Data")
    float ScoreScaled;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Data")
    float ScoreMin;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Data")
    float ScoreMax;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Data")
    int Duration;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Data")
    FXAPIExtension AdditionalContextData;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Data")
    FXAPIExtension AdditionalResultData;
};


USTRUCT(BlueprintType)
struct APEXSDK_API FCompleteSessionData : public FJsonable
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Complete Session")
    FString Uuid;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Complete Session")
    FString EventType;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Complete Session")
    int ModuleID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Complete Session")
    FString DeviceId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Complete Session")
    FXAPIStatement JsonData;

    // TODO: This data will get packed into the FXAPIStatement instead of an extra layer
    FSessionData SessionData;

    

    virtual ~FCompleteSessionData() { }

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

        Result->SetNumberField("moduleId", ModuleID);

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
            FString LessonStatus = "failed";
            if (SessionData.Complete)
            {
                LessonStatus = "passed";
            }
            JsonDataObject->SetStringField("lessonStatus", LessonStatus);
            JsonDataObject->SetStringField("moduleName", FString::FromInt(ModuleID));

            Result->SetObjectField("jsonData", JsonDataObject);
        }

        return Result;
    }
};

USTRUCT(BlueprintType)
struct APEXSDK_API FSessionEventData : public FJsonable
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Event")
    FString Uuid;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Event")
    FString EventType;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Event")
    int ModuleID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Event")
    FString DeviceId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Event")
    FXAPIStatement JsonData;

public:
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

        Result->SetNumberField("moduleId", ModuleID);

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
struct APEXSDK_API FAPEXOrganization : public FJsonable
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Apex Org")
    int ID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Apex Org")
    FString Name;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Apex Org")
    FString Status;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Apex Org")
    FString DownloadRegion;

public:
    virtual ~FAPEXOrganization() { }

    virtual void FromJsonObject(const TSharedPtr<FJsonObject>& JObject) override
    {
        ID = JObject->GetIntegerField("ID");
        Name = JObject->GetStringField("Name");
        Status = JObject->GetStringField("Status");
        DownloadRegion = JObject->GetStringField("DownloadRegion");
    }

    virtual TSharedPtr<FJsonObject> ToJsonObject() const override
    {
        TSharedPtr<FJsonObject> Result(new FJsonObject);

        Result->SetNumberField("ID", ID);
        Result->SetStringField("Name", Name);
        Result->SetStringField("Status", Status);
        Result->SetStringField("DownloadRegion", DownloadRegion);

        return Result;
    }
};

USTRUCT(BlueprintType)
struct APEXSDK_API FUserSessionData : public FJsonable
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|User Session Data")
    int ID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|User Session Data")
    int OrgId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|User Session Data")
    FString FirstName;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|User Session Data")
    FString LastName;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|User Session Data")
    FString Email;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|User Session Data")
    FString SessionToken;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|User Session Data")
    FAPEXOrganization Organization;

public:
    virtual ~FUserSessionData() { }

    bool IsSet()
    {
        return (!SessionToken.IsEmpty());
    }

    void Invalidate()
    {
        SessionToken = TEXT("");
    }

    virtual void FromJsonObject(const TSharedPtr<FJsonObject>& JObject) override
    {
        ID = JObject->GetIntegerField("ID");
        OrgId = JObject->GetIntegerField("OrgId");
        FirstName = JObject->GetStringField("First");
        LastName = JObject->GetStringField("Last");
        Email = JObject->GetStringField("Email");
        SessionToken = JObject->GetStringField("Token");
        Organization.FromJsonObject(JObject->GetObjectField("Org"));
    }

    virtual TSharedPtr<FJsonObject> ToJsonObject() const override
    {
        TSharedPtr<FJsonObject> Result(new FJsonObject);

        Result->SetNumberField("ID", ID);
        Result->SetNumberField("OrgId", OrgId);
        Result->SetStringField("First", FirstName);
        Result->SetStringField("Last", LastName);
        Result->SetStringField("Email", Email);
        Result->SetStringField("Token", SessionToken);
        Result->SetObjectField("Org", Organization.ToJsonObject());

        return Result;
    }
};