// Copyright 2022 PixoVR Corp. All Rights Reserved.

#include "ApexSDKSettings.h"

UApexSDKSettings::UApexSDKSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ServerURI = "";
	ModuleId = -1;
	MajorVerison = 1;
	MinorVersion = 0;
	PatchVersion = 0;
}
