// Fill out your copyright notice in the Description page of Project Settings.


#include "OptionalTypeLibrary.h"

int UOptionalLibrary::GetInt(const FOptionalInt& Optional)
{
	return Optional.Get();
}

float UOptionalLibrary::GetFloat(const FOptionalFloat& Optional)
{
	return Optional.Get();
}

bool UOptionalLibrary::GetBool(const FOptionalBool& Optional)
{
	return Optional.Get();
}

void UOptionalLibrary::SetInt(FOptionalInt& Optional, int Value)
{
	Optional.Set(Value);
}

void UOptionalLibrary::SetFloat(FOptionalFloat& Optional, float Value)
{
	Optional.Set(Value);
}

void UOptionalLibrary::SetBool(FOptionalBool& Optional, bool Value)
{
	Optional.Set(Value);
}

bool UOptionalLibrary::IsIntSet(const FOptionalInt& Optional)
{
	return Optional.IsSet();
}

bool UOptionalLibrary::IsFloatSet(const FOptionalFloat& Optional)
{
	return Optional.IsSet();
}

bool UOptionalLibrary::IsBoolSet(const FOptionalBool& Optional)
{
	return Optional.IsSet();
}