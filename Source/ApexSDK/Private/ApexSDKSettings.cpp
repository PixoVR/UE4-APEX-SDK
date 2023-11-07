// Copyright 2022 PixoVR Corp. All Rights Reserved.

#include "ApexSDKSettings.h"

#if WITH_EDITORONLY_DATA
#include "GeneralProjectSettings.h"
#include "AndroidRuntimeSettings.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogApexSDKSettings, Log, All);

#define LogAPEX(pmt, ...) UE_LOG(LogApexSDKSettings, Log, TEXT(pmt), ##__VA_ARGS__)
#define WarnAPEX(pmt, ...) UE_LOG(LogApexSDKSettings, Warning, TEXT(pmt), ##__VA_ARGS__)
#define ErrorAPEX(pmt, ...) UE_LOG(LogApexSDKSettings, Error, TEXT(pmt), ##__VA_ARGS__)
#define FatalAPEX(pmt, ...) UE_LOG(LogApexSDKSettings, Fatal, TEXT(pmt), ##__VA_ARGS__)

UApexSDKSettings::UApexSDKSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ServerURI = "";
	ModuleId = -1;
	ModuleVersion = "1.00.00";
	bSyncVersion = true;
}

bool UApexSDKSettings::CheckModuleVersion()
{
	TArray<FString> ModuleVersionArray;
	return GetModuleVersionArray(ModuleVersion, ModuleVersionArray);
}

bool UApexSDKSettings::GetModuleVersionArray(FString Version, TArray<FString>& ModuleVersionArray)
{
	int VersionPartCount = Version.ParseIntoArray(ModuleVersionArray, TEXT("."));

	if (VersionPartCount != 3)
	{
		return false;
	}

	if (ModuleVersionArray[0].Len() <= 0 || ModuleVersionArray[0].StartsWith("0") || !ModuleVersionArray[0].IsNumeric())
	{
		return false;
	}

	if (ModuleVersionArray[1].Len() != 2 || !ModuleVersionArray[1].IsNumeric())
	{
		return false;
	}

	if (ModuleVersionArray[2].Len() != 2 || !ModuleVersionArray[2].IsNumeric())
	{
		return false;
	}

	return true;
}

#if WITH_EDITORONLY_DATA
void UApexSDKSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.Property != nullptr)
	{
		const FName PropertyName(PropertyChangedEvent.Property->GetFName());
		if (PropertyName == GET_MEMBER_NAME_CHECKED(UApexSDKSettings, ModuleVersion) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(UApexSDKSettings, bSyncVersion) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(UApexSDKSettings, AdditionalVersionFields))
		{
			TArray<FString> ModuleVersionParts;
			if (GetModuleVersionArray(ModuleVersion, ModuleVersionParts))
			{
				if (bSyncVersion)
				{
					UpdateVersion(ModuleVersionParts, AdditionalVersionFields);
				}
			}
			else
			{
				ErrorAPEX("Module Version %s is invalid.", *ModuleVersion);
			}
		}
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UApexSDKSettings::UpdateVersion(const TArray<FString>& VersionParts, const TArray<EPVRVersionFields>& Fields)
{
	int MajorVerison = FCString::Atoi(*VersionParts[0]);
	int MinorVersion = FCString::Atoi(*VersionParts[1]);
	int PatchVersion = FCString::Atoi(*VersionParts[2]);

	FString FullVersion = FString::Printf(TEXT("%i.%.2i.%.2i"), MajorVerison, MinorVersion, PatchVersion);

	if (UGeneralProjectSettings* Settings = GetMutableDefault<UGeneralProjectSettings>())
	{
		Settings->ProjectVersion = FullVersion;
		Settings->UpdateSinglePropertyInConfigFile(Settings->GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UGeneralProjectSettings, ProjectVersion)), Settings->GetDefaultConfigFilename());
	}

	if (UAndroidRuntimeSettings* Settings = GetMutableDefault<UAndroidRuntimeSettings>())
	{
		Settings->VersionDisplayName = FullVersion;
		Settings->UpdateSinglePropertyInConfigFile(Settings->GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UAndroidRuntimeSettings, VersionDisplayName)), Settings->GetDefaultConfigFilename());

		if (Fields.Contains(EPVRVersionFields::AndroidStoreVersion))
		{
			FString AndroidVersionNumber = FString::Printf(TEXT("%i%.2i%.2i"), MajorVerison, MinorVersion, PatchVersion);
			Settings->StoreVersion = FCString::Atoi(*AndroidVersionNumber);
			Settings->UpdateSinglePropertyInConfigFile(Settings->GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UAndroidRuntimeSettings, StoreVersion)), Settings->GetDefaultConfigFilename());
		}
	}

	if (Fields.Contains(EPVRVersionFields::PixoVRPluginVersion))
	{
		if (GConfig)
		{
			static const FString DefaultGamePath = FString::Printf(TEXT("%sDefaultGame.ini"), *FPaths::SourceConfigDir());
			FString Setting;

			if (GConfig->GetString(TEXT("/Script/PixoVR.PixoVRSettings"), TEXT("ModuleVersion"), Setting, DefaultGamePath))
			{
				GConfig->SetString(TEXT("/Script/PixoVR.PixoVRSettings"), TEXT("ModuleVersion"), *FullVersion, DefaultGamePath);
			}

			GConfig->Flush(false, DefaultGamePath);
		}
	}
}
#endif

#undef FatalAPEX
#undef ErrorAPEX
#undef WarnAPEX
#undef LogAPEX