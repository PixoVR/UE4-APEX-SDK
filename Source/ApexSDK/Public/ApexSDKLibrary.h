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

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Apex|API")
	static bool GetCurrentSessionGuid(FGuid& OutCurrentSessionId);

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Apex|API")
	static FString GetClientIP();

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Apex|API")
	static FString GetDeviceModel();

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Apex|API")
	static FString GetDeviceId();

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Apex|API")
	static FString GetPlatform();

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Apex|API")
	static int GetModuleId();

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Apex|API")
	static FString GetModuleVersion();

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Apex|API")
	static bool GetCurrentUser(FUserSessionData& OutUser);
	
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Apex|API")
	static bool IsUserLoggedIn();

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Apex|API")
	static bool IsSessionInProgress();

public:

	static class UApexAPI* GetApexAPI();
	static FString GetAuthToken();
};