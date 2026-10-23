// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ApexTypes.h"
#include "Engine/DeveloperSettings.h"
#include "ApexSDKSettings.generated.h"

/** Extra version fields Sync Module Version writes. */
UENUM()
enum class EApexVersionField : uint8
{
	ANDROID_STORE_VERSION UMETA(DisplayName = "Android Store Version"),
};

/**
 * Project Settings > Plugins > Apex SDK, stored in DefaultEngine.ini. The module and version keys
 * are the ones the 1.x SDK used, so projects keep them when they upgrade.
 *
 * The subsystem reads these when the game instance starts; UApexSubsystem::ChangeEnvironment and
 * the setters there change them at runtime.
 */
UCLASS(Config = Engine, DefaultConfig, meta = (DisplayName = "Apex SDK"))
class APEXSDK_API UApexSDKSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UApexSDKSettings();

	virtual FName GetCategoryName() const override;

	// ------------------------------------------------------------ Module

	UPROPERTY(Config, EditAnywhere, Category = "Module", meta = (DisplayName = "Module ID"))
	int32 ModuleId;

	/** Sent with the session join data. */
	UPROPERTY(Config, EditAnywhere, Category = "Module")
	FString ModuleName;

	/** MAJOR.MINOR.PATCH, e.g. 1.04.00; the major may not start with 0. */
	UPROPERTY(Config, EditAnywhere, Category = "Module")
	FString ModuleVersion;

	/** Writes the module version into the project version and the Android version name. */
	UPROPERTY(Config, EditAnywhere, Category = "Module", meta = (DisplayName = "Sync Module Version"))
	bool bSyncVersion;

	UPROPERTY(Config, EditAnywhere, Category = "Module",
			  meta = (DisplayName = "Additional Synchronized Version Fields", EditCondition = "bSyncVersion"))
	TArray<EApexVersionField> AdditionalVersionFields;

	// ------------------------------------------------------------ Platform

	UPROPERTY(Config, EditAnywhere, Category = "Platform")
	EApexServerEnvironment Environment;

	/** Overrides the environment's Headset API host (login, access, sessions); required for Custom. */
	UPROPERTY(Config, EditAnywhere, Category = "Platform", meta = (DisplayName = "Headset API URL"))
	FString HeadsetApiUrl;

	/** Overrides the environment's Platform API host (GraphQL, token login, heartbeat); required for Custom. */
	UPROPERTY(Config, EditAnywhere, Category = "Platform", meta = (DisplayName = "Platform API URL"))
	FString PlatformApiUrl;

	/** On Android, pixoconfig.cnf in shared storage ({"Platform": "NA Stage"}) may pick the environment. */
	UPROPERTY(Config, EditAnywhere, Category = "Platform", meta = (DisplayName = "Read pixoconfig.cnf"))
	bool bReadPixoConfigFile;

	/** Sends requests through Unreal's HTTP module even where the core has its own (Windows, Android). */
	UPROPERTY(Config, EditAnywhere, Category = "Platform", AdvancedDisplay,
			  meta = (DisplayName = "Always Use Unreal HTTP"))
	bool bAlwaysUseUnrealHttp;

	UPROPERTY(Config, EditAnywhere, Category = "Platform", AdvancedDisplay, meta = (ClampMin = "1"))
	float RequestTimeoutSeconds;

	// ------------------------------------------------------------ Login

	/** After every login, checks the user's access to this module before reporting success. */
	UPROPERTY(Config, EditAnywhere, Category = "Login")
	bool bCheckModuleAccessOnLogin;

	/** Logs in with the launcher's token ("pixotoken") as soon as the game instance starts, if there is one. */
	UPROPERTY(Config, EditAnywhere, Category = "Login")
	bool bLoginWithLaunchTokenOnStart;

	// ------------------------------------------------------------ Sessions

	/** The scenario sessions report until JoinSession is given another. */
	UPROPERTY(Config, EditAnywhere, Category = "Sessions", meta = (DisplayName = "Default Scenario ID"))
	FString DefaultScenarioId;

	/**
	 * Session calls send the xAPI statements the platform reads today (the 1.x format). Turn off once
	 * the platform's new session route is live. A change applies from the next join.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Sessions", meta = (DisplayName = "Send xAPI Session Events"))
	bool bSendXapiSessionEvents;

	/** Session calls need a user whose module access was checked; turn off only for testing. */
	UPROPERTY(Config, EditAnywhere, Category = "Sessions")
	bool bRequireModuleAccessForSessions;

	/** Completes an active session (unsuccessful, no score) when the game shuts down. */
	UPROPERTY(Config, EditAnywhere, Category = "Sessions")
	bool bCompleteSessionOnShutdown;

	UPROPERTY(Config, EditAnywhere, Category = "Sessions", AdvancedDisplay, meta = (ClampMin = "1"))
	float HeartbeatIntervalSeconds;

	// ------------------------------------------------------------ Telemetry

	/** Turns off every analytics provider, the built-in PixoVR one included. */
	UPROPERTY(Config, EditAnywhere, Category = "Telemetry")
	bool bAnalyticsEnabled;

	/** The built-in provider: records tracked objects, gaze, interactions, steps and poses in the core. */
	UPROPERTY(Config, EditAnywhere, Category = "Telemetry",
			  meta = (DisplayName = "PixoVR Provider Enabled", EditCondition = "bAnalyticsEnabled"))
	bool bPixoVRProviderEnabled;

	/** Analytics providers, by name, that may not register. */
	UPROPERTY(Config, EditAnywhere, Category = "Telemetry", meta = (EditCondition = "bAnalyticsEnabled"))
	TArray<FString> DisabledProviders;

	UPROPERTY(Config, EditAnywhere, Category = "Telemetry", meta = (EditCondition = "bAnalyticsEnabled"))
	EApexTelemetrySinkType TelemetrySink;

	/** Where the HTTP sink POSTs packets; without it the packets are logged. */
	UPROPERTY(Config, EditAnywhere, Category = "Telemetry",
			  meta = (DisplayName = "Telemetry URL", EditCondition = "bAnalyticsEnabled"))
	FString TelemetryUrl;

	/** How often packets go out during a session. */
	UPROPERTY(Config, EditAnywhere, Category = "Telemetry", AdvancedDisplay, meta = (ClampMin = "1"))
	float TelemetryFlushIntervalSeconds;

	UPROPERTY(Config, EditAnywhere, Category = "Telemetry", AdvancedDisplay, meta = (ClampMin = "0.1"))
	float PoseSamplesPerSecond;

	UPROPERTY(Config, EditAnywhere, Category = "Telemetry", AdvancedDisplay, meta = (ClampMin = "1"))
	int32 MaximumRecordsPerPacket;

	/** True when the PixoVR provider may run: analytics on, provider on and not disabled by name, a sink. */
	bool IsPixoVRProviderEnabled() const;

	bool IsModuleVersionValid() const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;

private:
	void SyncProjectVersions() const;
#endif
};
