// Copyright 2022 PixoVR Corp. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PixoDataReporter.h"
#include "PixoDataReporterLibrary.generated.h"

/**
 * 
 */
UCLASS()
class PIXODATAREPORTING_API UPDataReporterLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "PixoVR|Data Reporter Library")
	static bool JoinSession(FString InScenarioId, const FXAPIExtension& InContextExtension, bool bIsMultiplayer = false);
	
	UFUNCTION(BlueprintCallable, Category = "PixoVR|Data Reporter Library")
	static bool CompleteSession(const FSessionData& InSessionData);
	
	UFUNCTION(BlueprintCallable, Category = "PixoVR|Data Reporter Library")
	static bool SendSessionEvent(const FXAPIStatement& InStatement);
	
	UFUNCTION(BlueprintCallable, Category = "PixoVR|Data Reporter Library")
	static bool FetchTopTen(FString Scenario, int OrgUnit);
	
	UFUNCTION(BlueprintCallable, Category = "PixoVR|Data Reporter Library", meta = (DisplayName = "Fetch Top Ten By User"))
	static bool FetchTopTenByUser(FString Scenario, int UserId = -1, int OrgUnit = -1);
	
	UFUNCTION(BlueprintCallable, Category = "PixoVR|Data Reporter Library")
	static bool FetchTopTenByTimeRange(float TimePriorToToday, FString Scenario, int OrgUnit);

public:

	static inline UPixoDataReporter* GetDataReporter()
	{
		static UPixoDataReporter* Subsystem = GEngine->GetEngineSubsystem<UPixoDataReporter>();
		return Subsystem;
	}
};