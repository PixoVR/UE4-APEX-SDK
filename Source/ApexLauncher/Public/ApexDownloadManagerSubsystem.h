// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ApexLauncherSubsystem.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ApexDownloadManagerSubsystem.generated.h"

UENUM(BlueprintType)
enum class EApexPackageOperationState : uint8
{
	WAITING,
	DOWNLOADING,
	EXTRACTING,
	INSTALLING,
	UNINSTALLING,
	COMPLETED,
	CANCELED,
	FAILED,
};

/** One install, update or uninstall the download manager follows. */
USTRUCT(BlueprintType)
struct APEXLAUNCHER_API FApexPackageOperation
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Download Manager")
	FString PackageName;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Download Manager")
	FString DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Download Manager")
	EApexPackageOperationState State = EApexPackageOperationState::WAITING;

	/** 0-100 within the current state. */
	UPROPERTY(BlueprintReadOnly, Category = "Apex|Download Manager")
	float ProgressPercent = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Apex|Download Manager")
	bool bIsUninstall = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FApexPackageOperationDelegate, const FApexPackageOperation&, Operation);

/**
 * Follows installs, updates and uninstalls started through it, one per package, and reports each
 * change of state or progress through OnOperationUpdated. A finished operation (COMPLETED,
 * CANCELED or FAILED) is reported once more and then forgotten.
 */
UCLASS(DisplayName = "Apex Download Manager")
class APEXLAUNCHER_API UApexDownloadManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& collection) override;
	virtual void Deinitialize() override;

	/** Downloads and installs a package (an update when it is installed). False when one is running. */
	UFUNCTION(BlueprintCallable, Category = "Apex|Download Manager")
	bool StartInstallation(const FString& displayName, const FString& packageName, const FString& url);

	UFUNCTION(BlueprintCallable, Category = "Apex|Download Manager")
	bool StartUninstallation(const FString& displayName, const FString& packageName);

	UFUNCTION(BlueprintPure, Category = "Apex|Download Manager")
	bool IsOperationRunning(const FString& packageName) const;

	UFUNCTION(BlueprintPure, Category = "Apex|Download Manager")
	TArray<FApexPackageOperation> GetRunningOperations() const;

	UPROPERTY(BlueprintAssignable, Category = "Apex|Download Manager")
	FApexPackageOperationDelegate OnOperationUpdated;

private:
	void HandlePackageEvent(const FString& packageName, EApexPackageEvent packageEvent, float progressPercent,
							int64 sizeBytes);
	void Update(FApexPackageOperation& operation, EApexPackageOperationState newState, float progressPercent);

	static FString MakeKey(const FString& packageName);

	TMap<FString, FApexPackageOperation> m_runningOperations;
	FDelegateHandle m_packageEventHandle;
};
