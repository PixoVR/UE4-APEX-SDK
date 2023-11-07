// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OptionalTypes.h"
#include "OptionalTypeLibrary.generated.h"

/**
 *
 */
UCLASS()
class APEXSDK_API UOptionalLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Apex|Optional Types|Int")
	static int GetInt(const FOptionalInt& Optional);

	UFUNCTION(BlueprintPure, Category = "Apex|Optional Types|Float")
	static float GetFloat(const FOptionalFloat& Optional);

	UFUNCTION(BlueprintPure, Category = "Apex|Optional Types|Bool")
	static bool GetBool(const FOptionalBool& Optional);

	UFUNCTION(BlueprintCallable, Category = "Apex|Optional Types|Int")
	static void SetInt(FOptionalInt& Optional, int Value);

	UFUNCTION(BlueprintCallable, Category = "Apex|Optional Types|Float")
	static void SetFloat(FOptionalFloat& Optional, float Value);

	UFUNCTION(BlueprintCallable, Category = "Apex|Optional Types|Bool")
	static void SetBool(FOptionalBool& Optional, bool Value);

	UFUNCTION(BlueprintPure, Category = "Apex|Optional Types|Int")
	static bool IsIntSet(const FOptionalInt& Optional);

	UFUNCTION(BlueprintPure, Category = "Apex|Optional Types|Float")
	static bool IsFloatSet(const FOptionalFloat& Optional);

	UFUNCTION(BlueprintPure, Category = "Apex|Optional Types|Bool")
	static bool IsBoolSet(const FOptionalBool& Optional);
public:
};
