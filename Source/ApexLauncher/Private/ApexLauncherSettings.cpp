// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#include "ApexLauncherSettings.h"

UApexLauncherSettings::UApexLauncherSettings() : HeadsetType(EApexHeadsetType::OTHER), bUseSystemSharedUserId(false)
{
}

FName UApexLauncherSettings::GetCategoryName() const
{
	return TEXT("Plugins");
}
