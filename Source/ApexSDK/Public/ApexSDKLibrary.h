// Copyright 2022 PixoVR Corp. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/GameInstance.h"
#include "ApexTypes.h"
#include "ApexSDKLibrary.generated.h"

/**
 * 
 */
UCLASS()
class APEXSDK_API UApexSDKLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Apex|API", meta = (WorldContext = "WorldContextObject"))
	static bool GetCurrentSessionGuid(UObject* WorldContextObject, FGuid& OutCurrentSessionId);

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Apex|API", meta = (WorldContext = "WorldContextObject"))
	static FString GetClientIP(UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Apex|API", meta = (WorldContext = "WorldContextObject"))
	static FString GetDeviceModel(UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Apex|API", meta = (WorldContext = "WorldContextObject"))
	static FString GetDeviceId(UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Apex|API", meta = (WorldContext = "WorldContextObject"))
	static FString GetPlatform(UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Apex|API", meta=(WorldContext = "WorldContextObject"))
	static int GetModuleId(UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Apex|API", meta=(WorldContext = "WorldContextObject"))
	static FString GetModuleVersion(UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Apex|API", meta=(WorldContext = "WorldContextObject"))
	static bool GetCurrentUser(UObject* WorldContextObject, FUserSessionData& OutUser);
	
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Apex|API", meta=(WorldContext = "WorldContextObject"))
	static bool IsUserLoggedIn(UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Apex|API", meta=(WorldContext = "WorldContextObject"))
	static bool IsSessionInProgress(UObject* WorldContextObject);

public:

	static class UApexAPI* GetApexAPI(UObject* WorldContextObject);
	static FString GetAuthToken(UObject* WorldContextObject);
};