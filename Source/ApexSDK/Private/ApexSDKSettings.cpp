// Copyright 2022 PixoVR Corp. All Rights Reserved.

#include "ApexSDKSettings.h"

#if WITH_EDITORONLY_DATA
#include "GeneralProjectSettings.h"
#include "AndroidRuntimeSettings.h"
#endif

UApexSDKSettings::UApexSDKSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ServerURI = "";
	ModuleId = -1;
	MajorVerison = 1;
	MinorVersion = 0;
	PatchVersion = 0;
	bSyncVersion = true;
}

#if WITH_EDITORONLY_DATA
void UApexSDKSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.Property != nullptr)
	{
		const FName PropertyName(PropertyChangedEvent.Property->GetFName());
		if (PropertyName == GET_MEMBER_NAME_CHECKED(UApexSDKSettings, MajorVerison) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(UApexSDKSettings, MinorVersion) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(UApexSDKSettings, PatchVersion) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(UApexSDKSettings, bSyncVersion) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(UApexSDKSettings, AdditionalVersionFields))
		{
			if (bSyncVersion)
			{
				UpdateVersion(AdditionalVersionFields);
			}
		}
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UApexSDKSettings::UpdateVersion(const TArray<TEnumAsByte<EPVRVersionFields>>& Fields)
{
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
	}

	if (Fields.Contains(EPVRVersionFields::AndroidStoreVersion))
	{
		FString AndroidVersionNumber = FString::Printf(TEXT("%i%.2i%.2i"), MajorVerison, MinorVersion, PatchVersion);
		if (UAndroidRuntimeSettings* Settings = GetMutableDefault<UAndroidRuntimeSettings>())
		{
			Settings->StoreVersion = FCString::Atoi(*AndroidVersionNumber);
			Settings->UpdateSinglePropertyInConfigFile(Settings->GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UAndroidRuntimeSettings, VersionDisplayName)), Settings->GetDefaultConfigFilename());
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
