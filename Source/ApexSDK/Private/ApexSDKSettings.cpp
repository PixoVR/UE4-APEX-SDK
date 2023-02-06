// Copyright 2022 PixoVR Corp. All Rights Reserved.

#include "ApexSDKSettings.h"
#include "GeneralProjectSettings.h"
#include "AndroidRuntimeSettings.h"

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
			PropertyName == GET_MEMBER_NAME_CHECKED(UApexSDKSettings, PatchVersion))
		{
			if (bSyncVersion)
			{
				UpdateVersion();
			}
		}
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UApexSDKSettings::UpdateVersion()
{
	FString FullVersion = FString::FromInt(MajorVerison) + "." + FString::FromInt(MinorVersion) + "." + FString::FromInt(PatchVersion);
	if (UGeneralProjectSettings* ProjectSettings = GetMutableDefault<UGeneralProjectSettings>())
	{
		ProjectSettings->ProjectVersion = FullVersion;
	}

	if (UAndroidRuntimeSettings* AndroidRuntimeSettings = GetMutableDefault<UAndroidRuntimeSettings>())
	{
		AndroidRuntimeSettings->VersionDisplayName = FullVersion;
	}
}
#endif