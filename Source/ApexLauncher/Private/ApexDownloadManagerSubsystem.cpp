// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#include "ApexDownloadManagerSubsystem.h"

void UApexDownloadManagerSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
	Super::Initialize(collection);
	if (UApexLauncherSubsystem* launcher = UApexLauncherSubsystem::Get())
	{
		m_packageEventHandle =
			launcher->OnPackageEventNative.AddUObject(this, &UApexDownloadManagerSubsystem::HandlePackageEvent);
	}
}

void UApexDownloadManagerSubsystem::Deinitialize()
{
	if (UApexLauncherSubsystem* launcher = UApexLauncherSubsystem::Get())
	{
		launcher->OnPackageEventNative.Remove(m_packageEventHandle);
	}

	m_runningOperations.Empty();
	Super::Deinitialize();
}

bool UApexDownloadManagerSubsystem::StartInstallation(const FString& displayName, const FString& packageName,
													  const FString& url)
{
	UApexLauncherSubsystem* launcher = UApexLauncherSubsystem::Get();
	if (launcher == nullptr || IsOperationRunning(packageName))
	{
		return false;
	}

	FApexPackageOperation newOperation;
	newOperation.PackageName = packageName;
	newOperation.DisplayName = displayName;
	m_runningOperations.Add(MakeKey(packageName), newOperation);
	if (!launcher->InstallApplication(GetGameInstance(), packageName, url))
	{
		m_runningOperations.Remove(MakeKey(packageName));
		return false;
	}

	OnOperationUpdated.Broadcast(newOperation);
	return true;
}

bool UApexDownloadManagerSubsystem::StartUninstallation(const FString& displayName, const FString& packageName)
{
	UApexLauncherSubsystem* launcher = UApexLauncherSubsystem::Get();
	if (launcher == nullptr || IsOperationRunning(packageName) || !launcher->IsApplicationInstalled(packageName))
	{
		return false;
	}

	FApexPackageOperation newOperation;
	newOperation.PackageName = packageName;
	newOperation.DisplayName = displayName;
	newOperation.bIsUninstall = true;
	m_runningOperations.Add(MakeKey(packageName), newOperation);
	if (!launcher->UninstallApplication(packageName))
	{
		m_runningOperations.Remove(MakeKey(packageName));
		return false;
	}

	OnOperationUpdated.Broadcast(newOperation);
	return true;
}

bool UApexDownloadManagerSubsystem::IsOperationRunning(const FString& packageName) const
{
	return m_runningOperations.Contains(MakeKey(packageName));
}

TArray<FApexPackageOperation> UApexDownloadManagerSubsystem::GetRunningOperations() const
{
	TArray<FApexPackageOperation> operations;
	m_runningOperations.GenerateValueArray(operations);
	return operations;
}

void UApexDownloadManagerSubsystem::HandlePackageEvent(const FString& packageName, EApexPackageEvent packageEvent,
													   float progressPercent, int64 /*sizeBytes*/)
{
	FApexPackageOperation* operation = m_runningOperations.Find(MakeKey(packageName));
	if (operation == nullptr)
	{
		return;
	}

	switch (packageEvent)
	{
		case EApexPackageEvent::DOWNLOAD_STARTED:
		case EApexPackageEvent::DOWNLOAD_PROGRESSED:
			Update(*operation, EApexPackageOperationState::DOWNLOADING, progressPercent);
			break;
		case EApexPackageEvent::EXTRACTION_STARTED:
		case EApexPackageEvent::EXTRACTION_PROGRESSED:
			Update(*operation, EApexPackageOperationState::EXTRACTING, progressPercent);
			break;
		case EApexPackageEvent::INSTALL_READY:
		case EApexPackageEvent::INSTALLATION_STARTED:
		case EApexPackageEvent::INSTALLATION_PROGRESSED:
			Update(*operation, EApexPackageOperationState::INSTALLING, progressPercent);
			break;
		case EApexPackageEvent::UNINSTALLATION_STARTED:
			Update(*operation, EApexPackageOperationState::UNINSTALLING, 0.0f);
			break;
		case EApexPackageEvent::INSTALLATION_COMPLETED:
		case EApexPackageEvent::UNINSTALLATION_COMPLETED:
			Update(*operation, EApexPackageOperationState::COMPLETED, 100.0f);
			break;
		case EApexPackageEvent::DOWNLOAD_CANCELED:
		case EApexPackageEvent::INSTALLATION_CANCELED:
		case EApexPackageEvent::UNINSTALLATION_CANCELED:
			Update(*operation, EApexPackageOperationState::CANCELED, operation->ProgressPercent);
			break;
		case EApexPackageEvent::DOWNLOAD_FAILED:
		case EApexPackageEvent::INSTALLATION_FAILED:
		case EApexPackageEvent::UNINSTALLATION_FAILED:
			Update(*operation, EApexPackageOperationState::FAILED, operation->ProgressPercent);
			break;
		default:
			// DOWNLOAD_COMPLETED and EXTRACTION_COMPLETED: the next stage's start follows.
			break;
	}
}

void UApexDownloadManagerSubsystem::Update(FApexPackageOperation& operation, EApexPackageOperationState newState,
										   float progressPercent)
{
	operation.State = newState;
	operation.ProgressPercent = progressPercent;
	const FApexPackageOperation reportedOperation = operation;
	const bool isFinished = newState == EApexPackageOperationState::COMPLETED ||
							newState == EApexPackageOperationState::CANCELED ||
							newState == EApexPackageOperationState::FAILED;
	if (isFinished)
	{
		// operation refers into the map; it is invalid after this.
		m_runningOperations.Remove(MakeKey(reportedOperation.PackageName));
	}

	OnOperationUpdated.Broadcast(reportedOperation);
}

FString UApexDownloadManagerSubsystem::MakeKey(const FString& packageName)
{
	return packageName.ToLower();
}
