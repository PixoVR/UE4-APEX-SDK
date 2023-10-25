// Copyright 2022 PixoVR Corp. All Rights Reserved.

#include "PApexSDK.h"
#include "ApexLibrary.h"
#include "ApexSDKDefines.h"

#include "UObject/Package.h"

#define LOCTEXT_NAMESPACE "FApexSDKModule"

DEFINE_LOG_CATEGORY_STATIC(LogApexSDKModule, Log, All);

#define Log(pmt, ...) UE_LOG(LogApexSDKModule, Log, TEXT(pmt), ##__VA_ARGS__)
#define Warn(pmt, ...) UE_LOG(LogApexSDKModule, Warning, TEXT(pmt), ##__VA_ARGS__)
#define Fatal(pmt, ...) UE_LOG(LogApexSDKModule, Fatal, TEXT(pmt), ##__VA_ARGS__)

void FApexSDKModule::StartupModule()
{
	Log("%s: ApexSDK (%s) module started", *APEX_FUNC_LINE, *UApexLibrary::GetApexSDKVersion());

	if (!FModuleManager::Get().IsModuleLoaded("WebSockets"))
	{
		Log("WebSockets module was not loading. Loading now.");
		FModuleManager::Get().LoadModule("WebSockets");
	}
}

void FApexSDKModule::ShutdownModule()
{
}

IMPLEMENT_MODULE(FApexSDKModule, ApexSDK)

#undef Log
#undef Warn
#undef Fatal

#undef LOCTEXT_NAMESPACE