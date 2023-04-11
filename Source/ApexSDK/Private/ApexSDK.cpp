// Copyright 2022 PixoVR Corp. All Rights Reserved.

#include "ApexSDK.h"

#include "ApexSDKSettings.h"
#include "ApexSDKDefines.h"

#include "Developer/Settings/Public/ISettingsModule.h"
#include "UObject/Package.h"

#define LOCTEXT_NAMESPACE "FApexSDKModule"

DEFINE_LOG_CATEGORY_STATIC(LogApexSDKModule, Log, All);

#define LogAPEX(pmt, ...) UE_LOG(LogApexSDKModule, Log, TEXT(pmt), ##__VA_ARGS__)
#define WarnAPEX(pmt, ...) UE_LOG(LogApexSDKModule, Warning, TEXT(pmt), ##__VA_ARGS__)
#define FatalAPEX(pmt, ...) UE_LOG(LogApexSDKModule, Fatal, TEXT(pmt), ##__VA_ARGS__)

void FApexSDKModule::StartupModule()
{
	ModuleSettings = NewObject<UApexSDKSettings>(GetTransientPackage(), "ApexSDKSettings", RF_Standalone);
	ModuleSettings->AddToRoot();

	// Register settings
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings("Project", "Plugins", "ApexSDK",
			LOCTEXT("RuntimeSettingsName", "Apex SDK"),
			LOCTEXT("RuntimeSettingsDescription", "Configure Apex SDK plugin settings"),
			ModuleSettings);
	}

	LogAPEX("%s: ApexSDK (%s) module started", *APEX_FUNC_LINE, *UApexLibrary::GetApexSDKVersion());
}

void FApexSDKModule::ShutdownModule()
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "ApexSDK");
	}

	if (!GExitPurge)
	{
		ModuleSettings->RemoveFromRoot();
	}
	else
	{
		ModuleSettings = nullptr;
	}
}

UApexSDKSettings* FApexSDKModule::GetSettings() const
{
	check(ModuleSettings);
	return ModuleSettings;
}

IMPLEMENT_MODULE(FApexSDKModule, ApexSDK)

#undef LogAPEX
#undef WarnAPEX
#undef FatalAPEX

#undef LOCTEXT_NAMESPACE