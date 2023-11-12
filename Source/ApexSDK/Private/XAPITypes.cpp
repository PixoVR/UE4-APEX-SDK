// Fill out your copyright notice in the Description page of Project Settings.


#include "XAPITypes.h"
#include "ApexLibrary.h"

using namespace Apex::XAPI;

// Define these here
TSharedPtr<FXAPIInteractionType> FXAPIInteractionType::Choice(new FXAPIInteractionType(CHOICE));
TSharedPtr<FXAPIInteractionType> FXAPIInteractionType::Sequencing(new FXAPIInteractionType(SEQUENCING));
TSharedPtr<FXAPIInteractionType> FXAPIInteractionType::Likert(new FXAPIInteractionType(LIKERT));
TSharedPtr<FXAPIInteractionType> FXAPIInteractionType::Matching(new FXAPIInteractionType(MATCHING));
TSharedPtr<FXAPIInteractionType> FXAPIInteractionType::Performance(new FXAPIInteractionType(PERFORMANCE));
TSharedPtr<FXAPIInteractionType> FXAPIInteractionType::TrueFalse(new FXAPIInteractionType(TRUEFALSE));
TSharedPtr<FXAPIInteractionType> FXAPIInteractionType::FillIn(new FXAPIInteractionType(FILLIN));
TSharedPtr<FXAPIInteractionType> FXAPIInteractionType::LongFillIn(new FXAPIInteractionType(LONGFILLIN));
TSharedPtr<FXAPIInteractionType> FXAPIInteractionType::Numeric(new FXAPIInteractionType(NUMERIC));
TSharedPtr<FXAPIInteractionType> FXAPIInteractionType::Other(new FXAPIInteractionType(OTHER));

TSharedPtr<FXAPIInteractionType> FXAPIInteractionType::FromValue(FString Value)
{
	if(Value.Equals(CHOICE))
		return Choice;
	else if (Value.Equals(SEQUENCING))
		return Sequencing;
	else if (Value.Equals(LIKERT))
		return Likert;
	else if (Value.Equals(MATCHING))
		return Matching;
	else if (Value.Equals(PERFORMANCE))
		return Performance;
	else if (Value.Equals(TRUEFALSE))
		return TrueFalse;
	else if (Value.Equals(FILLIN))
		return FillIn;
	else if (Value.Equals(LONGFILLIN))
		return LongFillIn;
	else if (Value.Equals(NUMERIC))
		return Numeric;
	else if (Value.Equals(OTHER))
		return Other;
	else
		return nullptr;
}

void FXAPIStatement::Stamp() const
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

bool FXAPIStatement::IsEmpty() const
{
	bool Empty = FXAPIStatementBase::IsEmpty() && !Id.IsValid() && !Stored.IsSet() && Authority.IsEmpty();
	return Empty;
}

void FXAPIStatement::FromJsonObject(const TSharedPtr<FJsonObject>& JObject)
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

TSharedPtr<FJsonObject> FXAPIStatement::ToJsonObject() const
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