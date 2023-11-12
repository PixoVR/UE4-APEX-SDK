// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ApexSDKDefines.h"
#include "ApexJSON.h"
#include "XAPITypes.h"
#include "ApexTypes.generated.h"

USTRUCT(BlueprintType)
struct APEXSDK_API FJoinSessionData : public FJsonable
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Join Session")
    FString Uuid = TEXT("");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Join Session")
    FString EventType = TEXT("");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Join Session")
    int ModuleId = -1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Join Session")
    FString DeviceId = TEXT("");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Join Session")
    FString IpAddress = TEXT("");
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
struct APEXSDK_API FAPEXRequestFailed : public FJsonable
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Failed Request")
    bool Error = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Failed Request")
    FString HttpCode = TEXT("0");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Failed Request")
    FString Message = TEXT("");

    FAPEXRequestFailed()
    {
        // Create a default
        Error = true;
        HttpCode = TEXT("400");
        Message = TEXT("There is a server connection error. Please check that there is a valid URL being used or check if APEX is down.");
    }

    virtual void FromJsonObject(const TSharedPtr<FJsonObject>& JObject) override
    {
        if(JObject->HasField("Error"))
            Error = JObject->GetBoolField("Error");

        if (JObject->HasField("HttpCode"))
            HttpCode = JObject->GetStringField("HttpCode");
    
        if (JObject->HasField("Message"))
            Message = JObject->GetStringField("Message");
    }
};

USTRUCT(BlueprintType)
struct APEXSDK_API FSessionData : public FJsonable
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Data")
    bool Complete = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Data")
    bool Success = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Data")
    float Score = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Data")
    float ScoreScaled = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Data")
    float ScoreMin = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Data")
    float ScoreMax = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Data")
    int Duration = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Data")
    FXAPIExtension AdditionalContextData;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Data")
    FXAPIExtension AdditionalResultData;

    virtual ~FSessionData() { }

    FSessionData() { }

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
struct APEXSDK_API FCompleteSessionData : public FJsonable
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Complete Session")
    FString Uuid = TEXT("");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Complete Session")
    FString EventType = TEXT("");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Complete Session")
    int ModuleId = -1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Complete Session")
    FString DeviceId = TEXT("");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Complete Session")
    FXAPIStatement JsonData;

    // TODO: This data will get packed into the FXAPIStatement instead of an extra layer
    FSessionData SessionData;

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
struct APEXSDK_API FSessionEventData : public FJsonable
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Event")
    FString Uuid = TEXT("");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Event")
    FString EventType = TEXT("");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Event")
    int ModuleId = -1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Event")
    FString DeviceId = TEXT("");
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
struct APEXSDK_API FAPEXOrganization : public FJsonable
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Apex Org")
    int ID = -1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Apex Org")
    FString Name = TEXT("");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Apex Org")
    FString Status = TEXT("");

public:
    virtual ~FAPEXOrganization() { }

    virtual void FromJsonObject(const TSharedPtr<FJsonObject>& JObject) override
    {
        ID = JObject->GetIntegerField("ID");
        Name = JObject->GetStringField("Name");
        Status = JObject->GetStringField("Status");
    }

    virtual TSharedPtr<FJsonObject> ToJsonObject() const override
    {
        TSharedPtr<FJsonObject> Result(new FJsonObject);

        Result->SetNumberField("ID", ID);
        Result->SetStringField("Name", Name);
        Result->SetStringField("Status", Status);

        return Result;
    }
};

USTRUCT(BlueprintType)
struct APEXSDK_API FUserSessionData : public FJsonable
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|User Session Data")
    int ID = -1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|User Session Data")
    int OrgId = -1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|User Session Data")
    int OrgUnit = -1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|User Session Data")
    FString FirstName = TEXT("");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|User Session Data")
    FString LastName = TEXT("");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|User Session Data")
    FString Email = TEXT("");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|User Session Data")
    FString SessionToken = TEXT("");
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
        if (JObject->HasField("OrgUnit"))
        {
            OrgUnit = JObject->GetIntegerField("OrgUnit");
        }
        else
        {
            OrgUnit = -1;
        }
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
        if (OrgUnit >= 0)
        {
            Result->SetNumberField("OrgUnit", OrgUnit);
        }
        Result->SetStringField("First", FirstName);
        Result->SetStringField("Last", LastName);
        Result->SetStringField("Email", Email);
        Result->SetStringField("Token", SessionToken);
        Result->SetObjectField("Org", Organization.ToJsonObject());

        return Result;
    }
};

USTRUCT(BlueprintType)
struct APEXSDK_API FApexUserData : public FJsonable
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|User Data")
    int Id = -1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|User Data")
    int OrgUnit = -1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|User Data")
    FString FirstName = TEXT("");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|User Data")
    FString LastName = TEXT("");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|User Data")
    FString Email = TEXT("");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|User Data")
    FString Username = TEXT("");

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
struct APEXSDK_API FSessionReportData : public FJsonable
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Report")
    FString Id = TEXT("");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Report")
    FString EventType = TEXT("");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Report")
    int UserId = -1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Report")
    int ModuleId = -1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Report")
    int SessionId = -1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Report")
    float SessionDuration = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Report")
    bool LessonStatus = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Report")
    FString CreatedAt = TEXT("");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Report")
    FString UpdatedAt = TEXT("");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Report")
    FXAPIStatement JsonData;

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
struct APEXSDK_API FTopTenData : public FJsonable
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Top Ten Data")
    bool Error = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Top Ten Data")
    FString Message = TEXT("");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Top Ten Data")
    FApexUserData User;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Top Ten Data")
    TArray<FSessionReportData> Sessions;

public:
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

        // TODO (MGruber): Fill this in later. It's not super important right now.

        return Result;
    }
};

