// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ApexJSON.h"
#include "XAPIBase.generated.h"

USTRUCT()
struct APEXSDK_API FXAPIBase : public FJsonable
{
	GENERATED_BODY()

	virtual ~FXAPIBase() {}

	virtual bool IsEmpty() const
	{
		return true;
	}
};