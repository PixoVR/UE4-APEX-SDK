// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Templates/SharedPointer.h"
#include "Misc/DateTime.h"
#include "Dom/JsonValue.h"
#include "ApexJSON.h"
#include "XAPIBase.h"
#include "XAPIExtension.h"
#include "ApexLibrary.h"
#include "OptionalTypes.h"
#include "XAPITypes.generated.h"

USTRUCT()
struct APEXSDK_API FXAPIStatementTarget
{
	GENERATED_BODY()

	virtual ~FXAPIStatementTarget() {}

	virtual FString GetObjectType()
	{
		return "";
	}
};

USTRUCT(BlueprintType)
struct APEXSDK_API FXAPILanguageMap : public FXAPIBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Language Map")
	TMap<FString, FString> Languages;

public:

	FXAPILanguageMap() {}

	FXAPILanguageMap(const TMap<FString, FString>& InLanguageMap)
	{
		Languages.Append(InLanguageMap);
	}

	FString Add(FString InLanguage, FString InValue)
	{
		return Languages.Add(InLanguage, InValue);
	}

	void Clone(const FXAPILanguageMap& InLanguageMap)
	{
		Languages.Empty();
		Languages.Append(InLanguageMap.Languages);
	}

	void Append(const FXAPILanguageMap& InLanguageMap)
	{
		Languages.Append(InLanguageMap.Languages);
	}

	virtual bool IsEmpty() const override
	{
		return Languages.Num() == 0;
	}

	virtual TSharedPtr<FJsonObject> ToJsonObject() const override
	{
		TSharedPtr<FJsonObject> Result(new FJsonObject);

		for (const TPair<FString, FString>& Pair : Languages)
		{
			Result->SetStringField(Pair.Key, Pair.Value);
		}

		return Result;
	}
};

namespace Apex
{
	namespace XAPI
	{
		static const FString CHOICE = TEXT("choice");
		static const FString SEQUENCING = TEXT("sequencing");
		static const FString LIKERT = TEXT("likert");
		static const FString MATCHING = TEXT("matching");
		static const FString PERFORMANCE = TEXT("performance");
		static const FString TRUEFALSE = TEXT("true-false");
		static const FString FILLIN = TEXT("fill-in");
		static const FString LONGFILLIN = TEXT("long-fill-in");
		static const FString NUMERIC = TEXT("numeric");
		static const FString OTHER = TEXT("other");
	}
}

USTRUCT()
struct APEXSDK_API FXAPIInteractionType
{
	GENERATED_BODY()

private:
	static TSharedPtr<FXAPIInteractionType> Choice;
	static TSharedPtr<FXAPIInteractionType> Sequencing;
	static TSharedPtr<FXAPIInteractionType> Likert;
	static TSharedPtr<FXAPIInteractionType> Matching;
	static TSharedPtr<FXAPIInteractionType> Performance;
	static TSharedPtr<FXAPIInteractionType> TrueFalse;
	static TSharedPtr<FXAPIInteractionType> FillIn;
	static TSharedPtr<FXAPIInteractionType> LongFillIn;
	static TSharedPtr<FXAPIInteractionType> Numeric;
	static TSharedPtr<FXAPIInteractionType> Other;

private:
	FString Value;

	FXAPIInteractionType(const FString Value)
	{
		this->Value = Value;
	}

public:

	// Do not create empty ones. This is requires for UE4 to work.
	FXAPIInteractionType() {}

	FString GetValue() const
	{
		return Value;
	}

	static TSharedPtr<FXAPIInteractionType> FromValue(FString Value);
};

USTRUCT(BlueprintType)
struct APEXSDK_API FXAPIInteractionComponent : public FXAPIBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Interaction Component")
	FString ID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Interaction Component")
	FXAPILanguageMap Description;


	virtual bool IsEmpty() const override
	{
		bool Empty = ID.IsEmpty() && Description.IsEmpty();
		return Empty;
	}

	virtual void FromJsonObject(const TSharedPtr<FJsonObject>& JObject) override
	{
		FXAPIBase::FromJsonObject(JObject);

		UApexLibrary::TryFillFromStringField(JObject, "id", ID);
		UApexLibrary::TryFillFromObjectField(JObject, "description", Description);
	}

	virtual TSharedPtr<FJsonObject> ToJsonObject() const override
	{
		TSharedPtr<FJsonObject> Result(new FJsonObject);

		if (!ID.IsEmpty())
		{
			Result->SetStringField("id", ID);
		}

		if (!Description.IsEmpty())
		{
			Result->SetObjectField("description", Description.ToJsonObject());
		}

		return Result;
	}
};

USTRUCT(BlueprintType)
struct APEXSDK_API FXAPIActivityDefinition : public FXAPIBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Activity Definition")
	FString Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Activity Definition")
	FString MoreInfo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Activity Definition")
	FXAPILanguageMap Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Activity Definition")
	FXAPILanguageMap Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Activity Definition")
	FXAPIExtension Extensions;

	// TODO (MGruber): Allow InteractionType to be set through a library function
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Activity Definition")
	TOptional<FXAPIInteractionType> InteractionType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Activity Definition")
	TArray<FString> CorrectResponsesPattern;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Activity Definition")
	TArray<FXAPIInteractionComponent> Choices;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Activity Definition")
	TArray<FXAPIInteractionComponent> Scale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Activity Definition")
	TArray<FXAPIInteractionComponent> Source;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Activity Definition")
	TArray<FXAPIInteractionComponent> Target;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Activity Definition")
	TArray<FXAPIInteractionComponent> Steps;

	virtual bool IsEmpty() const override
	{
		bool Empty = Type.IsEmpty() && MoreInfo.IsEmpty() && Name.IsEmpty();
		return Empty;
	}

	virtual void FromJsonObject(const TSharedPtr<FJsonObject>& JObject) override
	{
		FXAPIBase::FromJsonObject(JObject);

		UApexLibrary::TryFillFromStringField(JObject, "type", Type);
		UApexLibrary::TryFillFromStringField(JObject, "moreInfo", MoreInfo);
		FString InteractionTypeString;
		UApexLibrary::TryFillFromStringField(JObject, "interactionType", InteractionTypeString);
		InteractionType->FromValue(InteractionTypeString);
		UApexLibrary::TryFillFromObjectField(JObject, "name", Name);
		UApexLibrary::TryFillFromObjectField(JObject, "description", Description);
		UApexLibrary::TryFillFromObjectField(JObject, "extensions", Extensions);
		UApexLibrary::TryFillFromStringArrayField(JObject, "correctResponsesPattern", CorrectResponsesPattern);
		UApexLibrary::TryFillFromObjectArrayField(JObject, "choices", (TArray<FJsonable>&)Choices);
		UApexLibrary::TryFillFromObjectArrayField(JObject, "scale", (TArray<FJsonable>&)Scale);
		UApexLibrary::TryFillFromObjectArrayField(JObject, "source", (TArray<FJsonable>&)Source);
		UApexLibrary::TryFillFromObjectArrayField(JObject, "target", (TArray<FJsonable>&)Target);
		UApexLibrary::TryFillFromObjectArrayField(JObject, "steps", (TArray<FJsonable>&)Steps);
	}

	virtual TSharedPtr<FJsonObject> ToJsonObject() const override
	{
		TSharedPtr<FJsonObject> Result(new FJsonObject);

		if (!Type.IsEmpty())
		{
			Result->SetStringField("type", Type);
		}
		if (!MoreInfo.IsEmpty())
		{
			Result->SetStringField("moreInfo", MoreInfo);
		}
		if (!Name.IsEmpty())
		{
			Result->SetObjectField("name", Name.ToJsonObject());
		}
		if (!Description.IsEmpty())
		{
			Result->SetObjectField("description", Description.ToJsonObject());
		}
		if (!Extensions.IsEmpty())
		{
			Result->SetObjectField("extensions", Extensions.ToJsonObject());
		}
		if (InteractionType.IsSet())
		{
			Result->SetStringField("interactionType", InteractionType.GetValue().GetValue());
		}
		if (CorrectResponsesPattern.Num() > 0)
		{
			Result->SetArrayField("correctResponsesPattern", UApexLibrary::ConvertToJsonValueStringArray(CorrectResponsesPattern));
		}
		if (Choices.Num() > 0)
		{
			Result->SetArrayField("choices", UApexLibrary::ConvertToJsonValueObjectArray((TArray<FJsonable>)Choices));
		}
		if (Scale.Num() > 0)
		{
			Result->SetArrayField("scale", UApexLibrary::ConvertToJsonValueObjectArray((TArray<FJsonable>)Scale));
		}
		if (Source.Num() > 0)
		{
			Result->SetArrayField("source", UApexLibrary::ConvertToJsonValueObjectArray((TArray<FJsonable>)Source));
		}
		if (Target.Num() > 0)
		{
			Result->SetArrayField("target", UApexLibrary::ConvertToJsonValueObjectArray((TArray<FJsonable>)Target));
		}
		if (Steps.Num() > 0)
		{
			Result->SetArrayField("steps", UApexLibrary::ConvertToJsonValueObjectArray((TArray<FJsonable>)Steps));
		}

		return Result;
	}
};

USTRUCT(BlueprintType)
struct APEXSDK_API FXAPIAgent : public FXAPIBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Agent")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Agent")
	FString MBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Agent")
	FString MBoxSha1Sum;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Agent")
	FString OpenId;

	// TODO (MGruber): Add Account in as properties
	//AgentAccount Account;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Agent")
		FXAPIExtension Extensions;

	virtual bool IsEmpty() const override
	{
		bool Empty = Name.IsEmpty() && MBox.IsEmpty() && MBoxSha1Sum.IsEmpty() && OpenId.IsEmpty() && Extensions.IsEmpty();

		return Empty;
	}

	virtual FString GetObjectType() const
	{
		return TEXT("Agent");
	}

	virtual void FromJsonObject(const TSharedPtr<FJsonObject>& JObject) override
	{
		FXAPIBase::FromJsonObject(JObject);

		if (JObject->HasField("name"))
		{
			Name = JObject->GetStringField("name");
		}

		if (JObject->HasField("mbox"))
		{
			MBox = JObject->GetStringField("mbox");
		}

		if (JObject->HasField("mbox_sha1sum"))
		{
			MBoxSha1Sum = JObject->GetStringField("mbox_sha1sum");
		}

		if (JObject->HasField("openid"))
		{
			OpenId = JObject->GetStringField("openid");
		}

		if (JObject->HasField("extensions"))
		{
			Extensions.FromJsonObject(JObject->GetObjectField("extensions"));
		}
	}

	virtual TSharedPtr<FJsonObject> ToJsonObject() const override
	{
		TSharedPtr<FJsonObject> Result(new FJsonObject);

		if (!Name.IsEmpty())
		{
			Result->SetStringField("name", Name);
		}

		//if (account != null)
		//{
		//	result.Add("account", account.ToJObject(version));
		//}
		if (!MBox.IsEmpty())
		{
			Result->SetStringField("mbox", MBox);
		}
		else if (!MBoxSha1Sum.IsEmpty())
		{
			Result->SetStringField("mbox_sha1sum", MBoxSha1Sum);
		}
		else if (!OpenId.IsEmpty())
		{
			Result->SetStringField("openid", OpenId);
		}

		if (!Extensions.IsEmpty())
		{
			Result->SetObjectField("extensions", Extensions.ToJsonObject());
		}

		return Result;
	}
};

USTRUCT(BlueprintType)
struct APEXSDK_API FXAPIVerb : public FXAPIBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Verb")
	FString ID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Verb")
	FXAPILanguageMap Display;

	virtual bool IsEmpty() const override
	{
		bool Empty = ID.IsEmpty() && Display.IsEmpty();

		return Empty;
	}

	virtual void FromJsonObject(const TSharedPtr<FJsonObject>& JObject) override
	{
		FXAPIBase::FromJsonObject(JObject);

		if (JObject->HasField("id"))
		{
			ID = JObject->GetStringField("id");
		}

		if (JObject->HasField("display"))
		{
			Display.FromJsonObject(JObject->GetObjectField("display"));
		}
	}

	virtual TSharedPtr<FJsonObject> ToJsonObject() const override
	{
		TSharedPtr<FJsonObject> Result(new FJsonObject);

		if (!ID.IsEmpty())
		{
			Result->SetStringField("id", ID);
		}

		if (!Display.IsEmpty())
		{
			Result->SetObjectField("display", Display.ToJsonObject());
		}

		return Result;
	}
};

USTRUCT(BlueprintType)
struct APEXSDK_API FXAPIActivity : public FXAPIBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Activity")
	FString ID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Activity")
	FXAPIActivityDefinition Definition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Activity")
	FXAPIExtension Extensions;

	virtual FString GetObjectType() const
	{
		return TEXT("Activity");
	}

	virtual bool IsEmpty() const override
	{
		bool Empty = ID.IsEmpty() && Definition.IsEmpty() && Extensions.IsEmpty();

		return Empty;
	}

	virtual void FromJsonObject(const TSharedPtr<FJsonObject>& JObject) override
	{
		FXAPIBase::FromJsonObject(JObject);

		if (JObject->HasField("id"))
		{
			ID = JObject->GetStringField("id");
		}

		if (JObject->HasField("definition"))
		{
			Definition.FromJsonObject(JObject->GetObjectField("definition"));
		}

		if (JObject->HasField("extensions"))
		{
			Extensions.FromJsonObject(JObject->GetObjectField("extensions"));
		}
	}

	virtual TSharedPtr<FJsonObject> ToJsonObject() const override
	{
		TSharedPtr<FJsonObject> Result(new FJsonObject);

		if (!ID.IsEmpty())
		{
			Result->SetStringField("id", ID);
		}

		if (!Definition.IsEmpty())
		{
			Result->SetObjectField("definition", Definition.ToJsonObject());
		}

		if (!Extensions.IsEmpty())
		{
			Result->SetObjectField("extensions", Extensions.ToJsonObject());
		}

		return Result;
	}
};

USTRUCT(BlueprintType)
struct APEXSDK_API FXAPIContext : public FXAPIBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Context")
	FGuid Registration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Context")
	FXAPIAgent Instructor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Context")
	FXAPIAgent Team;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Context")
	FString Language;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Context")
	FString Revision;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Context")
	FString Platform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Context")
	FXAPIExtension Extensions;

	// TODO (MGruber): Add these in as properties
	//ContextActivities contextActivities;
	//StatementRef FStatement;

	virtual bool IsEmpty() const override
	{
		bool Empty = !Registration.IsValid() && Revision.IsEmpty() && Platform.IsEmpty() && Extensions.IsEmpty()
			&& Language.IsEmpty() && Team.IsEmpty() && Instructor.IsEmpty();

		return Empty;
	}

	virtual void FromJsonObject(const TSharedPtr<FJsonObject>& JObject) override
	{
		FXAPIBase::FromJsonObject(JObject);

		if (JObject->HasField("registration"))
		{
			FGuid::Parse(JObject->GetStringField("registration"), Registration);
		}

		if (JObject->HasField("instructor"))
		{
			Instructor.FromJsonObject(JObject->GetObjectField("instructor"));
		}

		if (JObject->HasField("team"))
		{
			Instructor.FromJsonObject(JObject->GetObjectField("team"));
		}

		if (JObject->HasField("revision"))
		{
			Revision = JObject->GetStringField("revision");
		}

		if (JObject->HasField("platform"))
		{
			Platform = JObject->GetStringField("platform");
		}

		if (JObject->HasField("language"))
		{
			Language = JObject->GetStringField("language");
		}

		if (JObject->HasField("extensions"))
		{
			Extensions.FromJsonObject(JObject->GetObjectField("extensions"));
		}
	}

	virtual TSharedPtr<FJsonObject> ToJsonObject() const override
	{
		TSharedPtr<FJsonObject> Result(new FJsonObject);

		if (Registration.IsValid())
		{
			Result->SetStringField("registration", Registration.ToString());
		}
		if (!Instructor.IsEmpty())
		{
			Result->SetObjectField("instructor", Instructor.ToJsonObject());
		}
		if (!Team.IsEmpty())
		{
			Result->SetObjectField("team", Team.ToJsonObject());
		}
		/*if (ContextActivities.IsEmpty())
		{
			Result->SetStringField("contextActivities", contextActivities.ToJObject(version));
		}*/
		if (!Revision.IsEmpty())
		{
			Result->SetStringField("revision", Revision);
		}
		if (!Platform.IsEmpty())
		{
			Result->SetStringField("platform", Platform);
		}
		if (!Language.IsEmpty())
		{
			Result->SetStringField("language", Language);
		}
		/*if (Statement.IsEmpty())
		{
			Result->SetStringField("statement", statement.ToJObject(version));
		}*/
		if (!Extensions.IsEmpty())
		{
			Result->SetObjectField("extensions", Extensions.ToJsonObject());
		}


		return Result;
	}
};

USTRUCT(BlueprintType)
struct APEXSDK_API FXAPIScore : public FXAPIBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Score")
	FOptionalFloat Scaled;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Score")
	FOptionalFloat Raw;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Score")
	FOptionalFloat Min;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Score")
	FOptionalFloat Max;

	virtual bool IsEmpty() const override
	{
		bool Empty = !Scaled.IsSet() && !Raw.IsSet() && !Min.IsSet() && !Max.IsSet();

		return Empty;
	}

	virtual void FromJsonObject(const TSharedPtr<FJsonObject>& JObject) override
	{
		FXAPIBase::FromJsonObject(JObject);

		if (JObject->HasField("scaled"))
		{
			Scaled = JObject->GetNumberField("scaled");
		}

		if (JObject->HasField("raw"))
		{
			Raw = JObject->GetNumberField("raw");
		}

		if (JObject->HasField("min"))
		{
			Min = JObject->GetNumberField("min");
		}

		if (JObject->HasField("max"))
		{
			Max = JObject->GetNumberField("max");
		}
	}

	virtual TSharedPtr<FJsonObject> ToJsonObject() const
	{
		TSharedPtr<FJsonObject> Result(new FJsonObject);

		if (Scaled.IsSet())
		{
			Result->SetNumberField("scaled", Scaled.Get());
		}

		if (Raw.IsSet())
		{
			Result->SetNumberField("raw", Raw.Get());
		}

		if (Min.IsSet())
		{
			Result->SetNumberField("min", Min.Get());
		}

		if (Max.IsSet())
		{
			Result->SetNumberField("max", Max.Get());
		}

		return Result;
	}
};

USTRUCT(BlueprintType)
struct FXAPIResult : public FXAPIBase
{
	GENERATED_BODY()

	TOptional<bool> Completion;

	TOptional<bool> Success;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Result")
	FString Response;

	TOptional<FTimespan> Duration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Result")
	FXAPIScore Score;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Result")
	FXAPIExtension Extensions;

	virtual bool IsEmpty() const override
	{
		bool Empty = !Completion.IsSet() && !Success.IsSet() && Response.IsEmpty() && !Duration.IsSet() && Score.IsEmpty() && Extensions.IsEmpty();

		return Empty;
	}

	virtual void FromJsonObject(const TSharedPtr<FJsonObject>& JObject) override
	{
		FXAPIBase::FromJsonObject(JObject);

		if (JObject->HasField("completion"))
		{
			Completion = JObject->GetBoolField("completion");
		}

		if (JObject->HasField("success"))
		{
			Success = JObject->GetBoolField("success");
		}

		if (JObject->HasField("response"))
		{
			Response = JObject->GetStringField("response");
		}

		if (JObject->HasField("duration"))
		{
			FString StoredString = JObject->GetStringField("duration");
			FTimespan Timespan;
			FTimespan::Parse(StoredString, Timespan);
			Duration = Timespan;
		}

		if (JObject->HasField("score"))
		{
			Score.FromJsonObject(JObject->GetObjectField("score"));
		}

		if (JObject->HasField("extensions"))
		{
			Extensions.FromJsonObject(JObject->GetObjectField("extensions"));
		}
	}

	virtual TSharedPtr<FJsonObject> ToJsonObject() const override
	{
		TSharedPtr<FJsonObject> Result(new FJsonObject);

		if (Completion.IsSet())
		{
			Result->SetBoolField("completion", Completion.GetValue());
		}

		if (Success.IsSet())
		{
			Result->SetBoolField("success", Success.GetValue());
		}

		if (!Response.IsEmpty())
		{
			Result->SetStringField("response", Response);
		}

		if (Duration.IsSet())
		{
			Result->SetStringField("duration", Duration.GetValue().ToString());
		}

		if (!Score.IsEmpty())
		{
			Result->SetObjectField("score", Score.ToJsonObject());
		}

		if (!Extensions.IsEmpty())
		{
			Result->SetObjectField("extensions", Extensions.ToJsonObject());
		}

		return Result;
	}
};


USTRUCT(BlueprintType)
struct APEXSDK_API FXAPIStatementBase : public FXAPIBase
{
	GENERATED_BODY()

protected:
	

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Statement")
	FXAPIAgent Actor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Statement")
	FXAPIVerb Verb;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Statement")
	FXAPIActivity Target;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Statement")
	FXAPIResult Result;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Statement")
	FXAPIContext Context;

	// TODO (MGruber): Add a function get the timestamp value.
	TOptional<FDateTime> TimeStamp;

	virtual bool IsEmpty() const override
	{
		bool Empty = Actor.IsEmpty() && Verb.IsEmpty() && Target.IsEmpty() && Result.IsEmpty()
			&& Context.IsEmpty() && !TimeStamp.IsSet();
		return Empty;
	}

	virtual void FromJsonObject(const TSharedPtr<FJsonObject>& JObject) override
	{
		FXAPIBase::FromJsonObject(JObject);

		if (JObject->HasField("actor"))
		{
			Actor.FromJsonObject(JObject->GetObjectField("actor"));
		}

		if (JObject->HasField("verb"))
		{
			Verb.FromJsonObject(JObject->GetObjectField("verb"));
		}

		if (JObject->HasField("object"))
		{
			Target.FromJsonObject(JObject->GetObjectField("object"));
		}

		if (JObject->HasField("result"))
		{
			Result.FromJsonObject(JObject->GetObjectField("result"));
		}

		if (JObject->HasField("context"))
		{
			Context.FromJsonObject(JObject->GetObjectField("context"));
		}

		if (JObject->HasField("timestamp"))
		{
			FString StoredString = JObject->GetStringField("timestamp");
			FDateTime StoredDateTime;
			FDateTime::Parse(StoredString, StoredDateTime);
			TimeStamp = StoredDateTime;
		}
	}

	virtual TSharedPtr<FJsonObject> ToJsonObject() const override
	{
		TSharedPtr<FJsonObject> JsonResult(new FJsonObject);

		if (!Actor.IsEmpty())
		{
			JsonResult->SetObjectField("actor", Actor.ToJsonObject());
		}

		if (!Verb.IsEmpty())
		{
			JsonResult->SetObjectField("verb", Verb.ToJsonObject());
		}

		if (!Target.IsEmpty())
		{
			JsonResult->SetObjectField("object", Target.ToJsonObject());
		}

		if (!Result.IsEmpty())
		{
			JsonResult->SetObjectField("result", Result.ToJsonObject());
		}

		if (!Context.IsEmpty())
		{
			JsonResult->SetObjectField("context", Context.ToJsonObject());
		}

		if (TimeStamp.IsSet())
		{
			JsonResult->SetStringField("timestamp", TimeStamp.GetValue().ToString());
		}

		return JsonResult;
	}
};

USTRUCT(BlueprintType)
struct APEXSDK_API FXAPIStatement : public FXAPIStatementBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Statement")
	mutable FGuid Id;

	mutable TOptional<FDateTime> Stored;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|XAPI|Statement")
	FXAPIAgent Authority;

	void Stamp() const
	{
		if (!Id.IsValid())
		{
			Id = FGuid::NewGuid();
		}

		if (!Stored.IsSet())
		{
			Stored = FDateTime::UtcNow();
		}
	}

	virtual bool IsEmpty() const override
	{
		bool Empty = FXAPIStatementBase::IsEmpty() && !Id.IsValid() && !Stored.IsSet() && Authority.IsEmpty();
		return Empty;
	}

	virtual void FromJsonObject(const TSharedPtr<FJsonObject>& JObject) override
	{
		FXAPIStatementBase::FromJsonObject(JObject);

		if (JObject->HasField("id"))
		{
			FGuid::Parse(JObject->GetStringField("id"), Id);
		}

		if (JObject->HasField("stored"))
		{
			FString StoredString = JObject->GetStringField("stored");
			FDateTime StoredDateTime;
			FDateTime::Parse(StoredString, StoredDateTime);
			Stored = StoredDateTime;
		}

		if (JObject->HasField("authority"))
		{
			Authority.FromJsonObject(JObject->GetObjectField("authority"));
		}
	}

	virtual TSharedPtr<FJsonObject> ToJsonObject() const override
	{
		TSharedPtr<FJsonObject> JsonResult = FXAPIStatementBase::ToJsonObject();

		if (Id.IsValid())
		{
			JsonResult->SetStringField("id", Id.ToString());
		}
		if (Stored.IsSet())
		{
			JsonResult->SetStringField("stored", Stored.GetValue().ToString());
		}
		if (!Authority.IsEmpty())
		{
			JsonResult->SetObjectField("authority", Authority.ToJsonObject());
		}
		
		JsonResult->SetStringField("version", TEXT("1.0.3"));

		return JsonResult;
	}
};
