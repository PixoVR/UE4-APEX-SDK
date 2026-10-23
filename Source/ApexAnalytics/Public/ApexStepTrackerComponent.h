// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ApexStepTrackerComponent.generated.h"

/** One named step of a scenario: call Begin, then Complete or Fail. Records step timing and score. */
UCLASS(ClassGroup = "Apex", meta = (BlueprintSpawnableComponent, DisplayName = "Apex Step Tracker"))
class APEXANALYTICS_API UApexStepTrackerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UApexStepTrackerComponent();

	UFUNCTION(BlueprintCallable, Category = "Apex|Analytics|Steps")
	void Begin();

	UFUNCTION(BlueprintCallable, Category = "Apex|Analytics|Steps")
	void Complete(float score = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Apex|Analytics|Steps")
	void Fail(float score = 0.0f);

	UFUNCTION(BlueprintPure, Category = "Apex|Analytics|Steps")
	FString GetStepName() const;

	UFUNCTION(BlueprintCallable, Category = "Apex|Analytics|Steps")
	void SetStepName(const FString& stepName);

private:
	UPROPERTY(EditAnywhere, Category = "Apex", meta = (DisplayName = "Step Name"))
	FString m_stepName;
};
