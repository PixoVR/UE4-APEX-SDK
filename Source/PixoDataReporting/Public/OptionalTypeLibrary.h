// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OptionalTypes.h"
#include "OptionalTypeLibrary.generated.h"

/**
 *
 */
UCLASS()
class PIXODATAREPORTING_API UOptionalLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "PixoVR|Optional Types|Int")
	static int GetInt(const FOptionalInt& Optional);

	UFUNCTION(BlueprintPure, Category = "PixoVR|Optional Types|Float")
	static float GetFloat(const FOptionalFloat& Optional);

	UFUNCTION(BlueprintPure, Category = "PixoVR|Optional Types|Bool")
	static bool GetBool(const FOptionalBool& Optional);

	UFUNCTION(BlueprintCallable, Category = "PixoVR|Optional Types|Int")
	static void SetInt(FOptionalInt& Optional, int Value);

	UFUNCTION(BlueprintCallable, Category = "PixoVR|Optional Types|Float")
	static void SetFloat(FOptionalFloat& Optional, float Value);

	UFUNCTION(BlueprintCallable, Category = "PixoVR|Optional Types|Bool")
	static void SetBool(FOptionalBool& Optional, bool Value);

	UFUNCTION(BlueprintPure, Category = "PixoVR|Optional Types|Int")
	static bool IsIntSet(const FOptionalInt& Optional);

	UFUNCTION(BlueprintPure, Category = "PixoVR|Optional Types|Float")
	static bool IsFloatSet(const FOptionalFloat& Optional);

	UFUNCTION(BlueprintPure, Category = "PixoVR|Optional Types|Bool")
	static bool IsBoolSet(const FOptionalBool& Optional);
public:
};
