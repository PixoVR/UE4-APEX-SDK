// Copyright 2022 PixoVR Corp. All Rights Reserved.

#include "ApexSDKLibrary.h"
#include "ApexAPI.h"
#include "Interfaces/IPluginManager.h"

#if WITH_EDITOR
#include "Editor/EditorEngine.h"
#endif

bool UApexSDKLibrary::GetCurrentSessionGuid(UObject* WorldContextObject, FGuid& OutCurrentSessionId)
{
	UApexAPI* Subsystem = GetApexAPI(WorldContextObject);
	if (Subsystem != nullptr)
	{
		OutCurrentSessionId = Subsystem->CurrentSessionGuid;
		return true;
	}

	return false;
}

FString UApexSDKLibrary::GetClientIP(UObject* WorldContextObject)
{
	UApexAPI* Subsystem = GetApexAPI(WorldContextObject);
	if (Subsystem != nullptr)
	{
		return Subsystem->ClientIP;
	}

	return TEXT("");
}

FString UApexSDKLibrary::GetDeviceModel(UObject* WorldContextObject)
{
	UApexAPI* Subsystem = GetApexAPI(WorldContextObject);
	if (Subsystem != nullptr)
	{
		return Subsystem->DeviceModel;
	}

	return TEXT("");
}

FString UApexSDKLibrary::GetDeviceId(UObject* WorldContextObject)
{
	UApexAPI* Subsystem = GetApexAPI(WorldContextObject);
	if (Subsystem != nullptr)
	{
		return Subsystem->DeviceId;
	}

	return TEXT("");
}

FString UApexSDKLibrary::GetPlatform(UObject* WorldContextObject)
{
	UApexAPI* Subsystem = GetApexAPI(WorldContextObject);
	if (Subsystem != nullptr)
	{
		return Subsystem->Platform;
	}

	return TEXT("");
}

int UApexSDKLibrary::GetModuleId(UObject* WorldContextObject)
{
	UApexAPI* Subsystem = GetApexAPI(WorldContextObject);
	if (Subsystem != nullptr)
	{
		return Subsystem->LoadedModuleId;
	}

	return -1;
}

FString UApexSDKLibrary::GetModuleVersion(UObject* WorldContextObject)
{
	UApexAPI* Subsystem = GetApexAPI(WorldContextObject);
	if (Subsystem != nullptr)
	{
		return Subsystem->ModuleVersion;
	}

	return TEXT("");
}


bool UApexSDKLibrary::GetCurrentUser(UObject* WorldContextObject, FUserSessionData& OutUser)
{
	bool bIsLoggedIn = false;

	UApexAPI* Subsystem = GetApexAPI(WorldContextObject);
	if (Subsystem != nullptr)
	{
		bIsLoggedIn = Subsystem->IsLoggedIn();
		if (bIsLoggedIn)
		{
			OutUser = Subsystem->CurrentActiveLogin;
		}
	}

	return bIsLoggedIn;
}

bool UApexSDKLibrary::IsUserLoggedIn(UObject* WorldContextObject)
{
	UApexAPI* Subsystem = GetApexAPI(WorldContextObject);
	if (Subsystem != nullptr)
	{
		return Subsystem->IsLoggedIn();
	}

	return false;
}

bool UApexSDKLibrary::IsSessionInProgress(UObject* WorldContextObject)
{
	UApexAPI* Subsystem = GetApexAPI(WorldContextObject);
	if (Subsystem != nullptr)
	{
		return Subsystem->bSessionInProgress;
	}

	return false;
}

FString UApexSDKLibrary::GetAuthToken(UObject* WorldContextObject)
{
	UApexAPI* Subsystem = GetApexAPI(WorldContextObject);
	if (Subsystem != nullptr)
	{
		if (Subsystem->IsLoggedIn())
		{
			return Subsystem->CurrentActiveLogin.SessionToken;
		}
	}

	return TEXT("");
}

UApexAPI* UApexSDKLibrary::GetApexAPI(UObject* WorldContextObject)
{
	UWorld* World = WorldContextObject->GetWorld();

	check(World);
	UGameInstance* GameInstance = World->GetGameInstance();

	check(GameInstance);

	return GameInstance->GetSubsystem<UApexAPI>();
}