// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#include "ApexSDKSettings.h"

#include "apex/Apex.h"

#if WITH_EDITOR
#include "AndroidRuntimeSettings.h"
#include "GeneralProjectSettings.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogApexSDKSettings, Log, All);

UApexSDKSettings::UApexSDKSettings()
	: ModuleId(-1),
	  ModuleName(TEXT("Generic")),
	  ModuleVersion(TEXT("1.00.00")),
	  bSyncVersion(true),
	  Environment(EApexServerEnvironment::NA_PRODUCTION),
	  bReadPixoConfigFile(true),
	  bAlwaysUseUnrealHttp(false),
	  RequestTimeoutSeconds(30.0f),
	  bCheckModuleAccessOnLogin(true),
	  bLoginWithLaunchTokenOnStart(false),
	  DefaultScenarioId(TEXT("Generic")),
	  bSendXapiSessionEvents(true),
	  bRequireModuleAccessForSessions(true),
	  bCompleteSessionOnShutdown(true),
	  HeartbeatIntervalSeconds(5.0f),
	  bAnalyticsEnabled(true),
	  bPixoVRProviderEnabled(true),
	  TelemetrySink(EApexTelemetrySinkType::LOG),
	  TelemetryFlushIntervalSeconds(10.0f),
	  PoseSamplesPerSecond(2.0f),
	  MaximumRecordsPerPacket(500)
{
}

FName UApexSDKSettings::GetCategoryName() const
{
	return TEXT("Plugins");
}

bool UApexSDKSettings::IsPixoVRProviderEnabled() const
{
	return bAnalyticsEnabled && bPixoVRProviderEnabled && !DisabledProviders.Contains(TEXT("PixoVR")) &&
		   TelemetrySink != EApexTelemetrySinkType::NONE;
}

bool UApexSDKSettings::IsModuleVersionValid() const
{
	return ApexModuleVersionIsValid(TCHAR_TO_UTF8(*ModuleVersion)) != 0;
}

#if WITH_EDITOR
void UApexSDKSettings::PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent)
{
	Super::PostEditChangeProperty(propertyChangedEvent);

	const FName propertyName = propertyChangedEvent.GetPropertyName();
	const bool isVersionProperty = propertyName == GET_MEMBER_NAME_CHECKED(UApexSDKSettings, ModuleVersion) ||
								   propertyName == GET_MEMBER_NAME_CHECKED(UApexSDKSettings, bSyncVersion) ||
								   propertyName == GET_MEMBER_NAME_CHECKED(UApexSDKSettings, AdditionalVersionFields);
	if (!isVersionProperty)
	{
		return;
	}

	if (!IsModuleVersionValid())
	{
		UE_LOG(LogApexSDKSettings, Error, TEXT("Module version %s is not MAJOR.MINOR.PATCH."), *ModuleVersion);
		return;
	}

	if (bSyncVersion)
	{
		SyncProjectVersions();
	}
}

void UApexSDKSettings::SyncProjectVersions() const
{
	TArray<FString> versionParts;
	ModuleVersion.ParseIntoArray(versionParts, TEXT("."));
	const int32 majorVersion = FCString::Atoi(*versionParts[0]);
	const int32 minorVersion = FCString::Atoi(*versionParts[1]);
	const int32 patchVersion = FCString::Atoi(*versionParts[2]);
	const FString fullVersion = FString::Printf(TEXT("%d.%02d.%02d"), majorVersion, minorVersion, patchVersion);

	if (UGeneralProjectSettings* projectSettings = GetMutableDefault<UGeneralProjectSettings>())
	{
		projectSettings->ProjectVersion = fullVersion;
		projectSettings->UpdateSinglePropertyInConfigFile(
			projectSettings->GetClass()->FindPropertyByName(
				GET_MEMBER_NAME_CHECKED(UGeneralProjectSettings, ProjectVersion)),
			projectSettings->GetDefaultConfigFilename());
	}

	if (UAndroidRuntimeSettings* androidSettings = GetMutableDefault<UAndroidRuntimeSettings>())
	{
		androidSettings->VersionDisplayName = fullVersion;
		androidSettings->UpdateSinglePropertyInConfigFile(
			androidSettings->GetClass()->FindPropertyByName(
				GET_MEMBER_NAME_CHECKED(UAndroidRuntimeSettings, VersionDisplayName)),
			androidSettings->GetDefaultConfigFilename());

		if (AdditionalVersionFields.Contains(EApexVersionField::ANDROID_STORE_VERSION))
		{
			androidSettings->StoreVersion = majorVersion * 10000 + minorVersion * 100 + patchVersion;
			androidSettings->UpdateSinglePropertyInConfigFile(
				androidSettings->GetClass()->FindPropertyByName(
					GET_MEMBER_NAME_CHECKED(UAndroidRuntimeSettings, StoreVersion)),
				androidSettings->GetDefaultConfigFilename());
		}
	}
}
#endif
