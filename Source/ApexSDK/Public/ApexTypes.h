// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ApexTypes.generated.h"

// The C interface (apex/Apex.h) already uses the names EApexError, EApexEnvironment, EApexLifecycle,
// EApexModuleSort and EApexTelemetrySink, so the Unreal types below are named differently.

/** Which Apex platform the SDK talks to. Values match the core's EApexEnvironment. */
UENUM(BlueprintType)
enum class EApexServerEnvironment : uint8
{
	NA_PRODUCTION = 0 UMETA(DisplayName = "North America - Production"),
	NA_STAGING = 1 UMETA(DisplayName = "North America - Staging"),
	NA_DEVELOPMENT = 2 UMETA(DisplayName = "North America - Development"),
	SA_PRODUCTION = 3 UMETA(DisplayName = "Saudi Arabia - Production"),
	LOCAL = 4 UMETA(DisplayName = "Local (localhost:8001 and :8000)"),
	CUSTOM = 5 UMETA(DisplayName = "Custom (both URLs from the settings)"),
};

/** Why a call failed. Values match the core's EApexError. */
UENUM(BlueprintType)
enum class EApexErrorCode : uint8
{
	OK = 0,
	NETWORK = 1 UMETA(ToolTip = "No HTTP response: DNS, TLS, timeout, offline"),
	HTTP = 2 UMETA(ToolTip = "HTTP status outside 2xx with no platform error body"),
	PARSE = 3 UMETA(ToolTip = "The response was not what the platform should send"),
	PLATFORM = 4 UMETA(ToolTip = "The platform reported an error; see the message"),
	NOT_LOGGED_IN = 5,
	CANCELLED = 6,
	INVALID_ARGUMENT = 7,
	INVALID_STATE = 8 UMETA(ToolTip = "For example an event sent with no active session"),
	NO_TRANSPORT = 9 UMETA(ToolTip = "No way to send HTTP requests on this platform"),
	INTERNAL = 10,
};

/** Module version lifecycles, as flags. Values match the core's EApexLifecycle. */
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EApexModuleLifecycle : uint8
{
	NONE = 0 UMETA(Hidden),
	RELEASED = 1,
	DEVELOPMENT = 2,
	QA = 4,
	UAT = 8,
	OTHER = 16 UMETA(ToolTip = "Any other lifecycle name, or none"),
};
ENUM_CLASS_FLAGS(EApexModuleLifecycle);

/** Order of a module page. Values match the core's EApexModuleSort. */
UENUM(BlueprintType)
enum class EApexModuleSortOrder : uint8
{
	PLATFORM = 0 UMETA(ToolTip = "The order the platform returns"),
	NAME = 1 UMETA(ToolTip = "Name A-Z, ignoring case"),
	ID = 2 UMETA(ToolTip = "Module ID, lowest first"),
	NEWEST_VERSION = 3 UMETA(ToolTip = "Newest matching version first"),
};

/** Where telemetry packets go. */
UENUM(BlueprintType)
enum class EApexTelemetrySinkType : uint8
{
	NONE UMETA(ToolTip = "Nothing is recorded"),
	LOG UMETA(ToolTip = "Each packet is written to the log"),
	FILE UMETA(ToolTip = "Each packet is saved under Saved/ApexTelemetry"),
	HTTP UMETA(ToolTip = "The core POSTs each packet to the telemetry URL with the user's token"),
};

/** The outcome of a call: what every result carries. */
USTRUCT(BlueprintType)
struct APEXSDK_API FApexResponse
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Apex")
	EApexErrorCode Error = EApexErrorCode::OK;

	/** 0 when no HTTP response was received, including answers the core served from its cache. */
	UPROPERTY(BlueprintReadOnly, Category = "Apex")
	int32 HttpStatus = 0;

	/** Empty on success. */
	UPROPERTY(BlueprintReadOnly, Category = "Apex")
	FString Message;

	bool IsSuccess() const
	{
		return Error == EApexErrorCode::OK;
	}

	static FApexResponse MakeFailure(EApexErrorCode error, const FString& message);
};

USTRUCT(BlueprintType)
struct APEXSDK_API FApexOrganization
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Apex|User")
	int32 OrganizationId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|User")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|User")
	FString Status;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|User")
	FString DownloadRegion;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|User")
	FString Type;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|User")
	FString HubLogoLink;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|User")
	FString PrimaryColor;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|User")
	FString SecondaryColor;
};

/** A logged-in user (spec/schema/user.json in apex-sdk-core). */
USTRUCT(BlueprintType)
struct APEXSDK_API FApexUser
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Apex|User")
	int32 UserId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|User")
	int32 OrganizationId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|User")
	FString FirstName;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|User")
	FString LastName;

	/** Only Quick ID logins return it. */
	UPROPERTY(BlueprintReadOnly, Category = "Apex|User")
	FString Username;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|User")
	FString Email;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|User")
	FString Role;

	/** The bearer token; launching another module passes it on. Never log it. */
	UPROPERTY(BlueprintReadOnly, Category = "Apex|User")
	FString Token;

	/** Set from the module access check when the platform sends a passing score. */
	UPROPERTY(BlueprintReadOnly, Category = "Apex|User")
	float MinimumPassingScore = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|User")
	FApexOrganization Organization;
};

/** The current user's access to a module. */
USTRUCT(BlueprintType)
struct APEXSDK_API FApexModuleAccess
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	int32 UserId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	int32 ModuleId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	bool bHasAccess = false;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	bool bHasPassingScore = false;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	float PassingScore = 0.0f;
};

USTRUCT(BlueprintType)
struct APEXSDK_API FApexQuickIdUser
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Quick ID")
	FString FirstName;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Quick ID")
	FString LastName;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Quick ID")
	FString Username;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Quick ID")
	FString Email;
};

/** The users enrolled for Quick ID on a device, and their organization's branding. */
USTRUCT(BlueprintType)
struct APEXSDK_API FApexQuickIdUsers
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Quick ID")
	FString OrganizationName;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Quick ID")
	FString PrimaryColor;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Quick ID")
	FString SecondaryColor;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Quick ID")
	FString HubLogoUrl;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Quick ID")
	TArray<FApexQuickIdUser> Users;
};

/** A one-time login code. */
USTRUCT(BlueprintType)
struct APEXSDK_API FApexLoginCode
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Apex|User")
	FString Code;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|User")
	FString ExpiresAt;
};

USTRUCT(BlueprintType)
struct APEXSDK_API FApexUserModules
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	int32 UserId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	TArray<int32> ModuleIds;
};

USTRUCT(BlueprintType)
struct APEXSDK_API FApexModuleLifecycleInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	int32 LifecycleId = 0;

	/** As the platform names it; empty when the version has no lifecycle. */
	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	EApexModuleLifecycle Stage = EApexModuleLifecycle::OTHER;
};

USTRUCT(BlueprintType)
struct APEXSDK_API FApexModuleVersion
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	int32 VersionId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	FString Version;

	/** Android package name. */
	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	FString Package;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	FString CreatedAt;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	TArray<FString> Controls;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	TArray<FString> Platforms;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	FApexModuleLifecycleInfo Lifecycle;
};

USTRUCT(BlueprintType)
struct APEXSDK_API FApexModulePlayerVersion
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	int32 VersionId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	FString Version;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	FString Status;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	FString Package;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	TArray<FString> Platforms;
};

/** The app that plays a module's content, when it has one. */
USTRUCT(BlueprintType)
struct APEXSDK_API FApexModulePlayer
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	bool bIsSet = false;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	int32 PlayerId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	FString LaunchProtocol;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	TArray<FApexModulePlayerVersion> Versions;
};

/** A module available to the user (spec/schema/module_list.json in apex-sdk-core). */
USTRUCT(BlueprintType)
struct APEXSDK_API FApexModule
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	int32 ModuleId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	FString Abbreviation;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	FString ExternalId;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	FString IconUrl;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	FString Distributor;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	FString ShortDescription;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	FString LongDescription;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	FString Industry;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	FString Details;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	FString Categories;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	bool bIsAvailable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	bool bIsAuthenticatedLaunch = false;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	TArray<FString> Languages;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	FApexModulePlayer Player;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	TArray<FApexModuleVersion> Versions;
};

/** One page of modules (spec/schema/module_page.json in apex-sdk-core). */
USTRUCT(BlueprintType)
struct APEXSDK_API FApexModulePage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	TArray<FApexModule> Modules;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	int32 PageIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	int32 PageSize = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	int32 PageCount = 0;

	/** Modules matching the filters, across every page. */
	UPROPERTY(BlueprintReadOnly, Category = "Apex|Modules")
	int32 TotalCount = 0;
};

/**
 * How a session ended, sent with CompleteSession. With ScaledScore 0, a score and a maximum
 * score, the SDK sends Score / MaximumScore * 100 as the scaled score.
 */
USTRUCT(BlueprintType)
struct APEXSDK_API FApexSessionResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Sessions")
	float Score = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Sessions")
	float ScaledScore = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Sessions")
	float MinimumScore = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Sessions")
	float MaximumScore = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Sessions")
	float DurationSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Sessions")
	bool bComplete = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Sessions")
	bool bSuccess = false;

	float GetEffectiveScaledScore() const;
};
