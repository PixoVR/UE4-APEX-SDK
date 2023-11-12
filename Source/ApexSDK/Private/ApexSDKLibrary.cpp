// Copyright 2022 PixoVR Corp. All Rights Reserved.

#include "ApexSDKLibrary.h"
#include "ApexAPI.h"
#include "Interfaces/IPluginManager.h"

#if WITH_EDITOR
#include "Editor/EditorEngine.h"
#endif

bool UApexSDKLibrary::GetCurrentSessionGuid(FGuid& OutCurrentSessionId)
{
	UApexAPI* Subsystem = GetApexAPI();
	if (Subsystem != nullptr)
	{
		OutCurrentSessionId = Subsystem->CurrentSessionGuid;
		return true;
	}

	return false;
}

FString UApexSDKLibrary::GetClientIP()
{
	UApexAPI* Subsystem = GetApexAPI();
	if (Subsystem != nullptr)
	{
		return Subsystem->ClientIP;
	}

	return TEXT("");
}

FString UApexSDKLibrary::GetDeviceModel()
{
	UApexAPI* Subsystem = GetApexAPI();
	if (Subsystem != nullptr)
	{
		return Subsystem->DeviceModel;
	}

	return TEXT("");
}

FString UApexSDKLibrary::GetDeviceId()
{
	UApexAPI* Subsystem = GetApexAPI();
	if (Subsystem != nullptr)
	{
		return Subsystem->DeviceId;
	}

	return TEXT("");
}

FString UApexSDKLibrary::GetPlatform()
{
	UApexAPI* Subsystem = GetApexAPI();
	if (Subsystem != nullptr)
	{
		return Subsystem->Platform;
	}

	return TEXT("");
}

int UApexSDKLibrary::GetModuleId()
{
	UApexAPI* Subsystem = GetApexAPI();
	if (Subsystem != nullptr)
	{
		return Subsystem->LoadedModuleId;
	}

	return -1;
}

FString UApexSDKLibrary::GetModuleVersion()
{
	UApexAPI* Subsystem = GetApexAPI();
	if (Subsystem != nullptr)
	{
		return Subsystem->ModuleVersion;
	}

	return TEXT("");
}


bool UApexSDKLibrary::GetCurrentUser(FUserSessionData& OutUser)
{
	bool bIsLoggedIn = false;

	UApexAPI* Subsystem = GetApexAPI();
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

bool UApexSDKLibrary::IsUserLoggedIn()
{
	UApexAPI* Subsystem = GetApexAPI();
	if (Subsystem != nullptr)
	{
		return Subsystem->IsLoggedIn();
	}

	return false;
}

bool UApexSDKLibrary::IsSessionInProgress()
{
	UApexAPI* Subsystem = GetApexAPI();
	if (Subsystem != nullptr)
	{
		return Subsystem->bSessionInProgress;
	}

	return false;
}

FString UApexSDKLibrary::GetAuthToken()
{
	UApexAPI* Subsystem = GetApexAPI();
	if (Subsystem != nullptr)
	{
		if (Subsystem->IsLoggedIn())
		{
			return Subsystem->CurrentActiveLogin.SessionToken;
		}
	}

	return TEXT("");
}

UApexAPI* UApexSDKLibrary::GetApexAPI()
{
	UWorld* World = nullptr;
#if WITH_EDITOR
	UEditorEngine* EEngine = Cast<UEditorEngine>(GEngine);
	if (GIsEditor && EEngine != nullptr)
	{
#if ENGINE_MAJOR_VERSION >= 5
		// lets use PlayWorld during PIE/Simulate and regular world from editor otherwise, to draw debug information
		World = EEngine->PlayWorld != nullptr ? ToRawPtr(EEngine->PlayWorld) : EEngine->GetEditorWorldContext().World();
#else
		World = EEngine->PlayWorld != nullptr ? EEngine->PlayWorld : EEngine->GetEditorWorldContext().World();
#endif
	}
#endif

	if (!GIsEditor && World == nullptr && GEngine != nullptr)
	{
		World = GEngine->GetWorld();
	}

	check(World);
	UGameInstance* GameInstance = World->GetGameInstance();

	check(GameInstance);

	return GameInstance->GetSubsystem<UApexAPI>();
}