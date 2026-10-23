// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "Templates/SubclassOf.h"
#include "ApexLauncherSubsystem.generated.h"

class AActor;

/** Something that happened to a package being downloaded, installed or uninstalled. */
UENUM(BlueprintType)
enum class EApexPackageEvent : uint8
{
	DOWNLOAD_STARTED,
	DOWNLOAD_PROGRESSED,
	DOWNLOAD_COMPLETED,
	DOWNLOAD_CANCELED,
	DOWNLOAD_FAILED,
	EXTRACTION_STARTED,
	EXTRACTION_PROGRESSED,
	EXTRACTION_COMPLETED,
	INSTALL_READY,
	INSTALLATION_STARTED,
	INSTALLATION_PROGRESSED,
	INSTALLATION_COMPLETED,
	INSTALLATION_CANCELED,
	INSTALLATION_FAILED,
	UNINSTALLATION_STARTED,
	UNINSTALLATION_COMPLETED,
	UNINSTALLATION_CANCELED,
	UNINSTALLATION_FAILED,
};

/**
 * PackageName is the Android package (or, for downloads, the name it was requested with).
 * ProgressPercent is 0-100 for the PROGRESSED events, SizeBytes is set for DOWNLOAD_STARTED and
 * INSTALL_READY; both are 0 otherwise.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FApexPackageEventDelegate, const FString&, PackageName, EApexPackageEvent,
											  PackageEvent, float, ProgressPercent, int64, SizeBytes);
DECLARE_MULTICAST_DELEGATE_FourParams(FApexPackageEventNativeDelegate, const FString&, EApexPackageEvent, float, int64);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FApexMockDownloaderSpawnedDelegate, AActor*, MockDownloader,
											   const FString&, PackageName, const FString&, Url);

/**
 * Launches, installs, updates and uninstalls applications, and reads how this one was launched.
 *
 * Android does the work in Java (ApexLauncherAndroid_UPL.xml); installs report progress through
 * OnPackageEvent on the game thread. Elsewhere, launching URLs works, launching apps does not, and
 * installs spawn the mock downloader actor (SetMockDownloaderClass) so a UI can be tested.
 */
UCLASS(DisplayName = "Apex Launcher")
class APEXLAUNCHER_API UApexLauncherSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& collection) override;
	virtual void Deinitialize() override;

	/** The running subsystem, or nullptr before the engine starts or after it shuts down. */
	static UApexLauncherSubsystem* Get();

	// ------------------------------------------------------------ Launching

	UFUNCTION(BlueprintPure, Category = "Apex|Launcher")
	bool IsApplicationInstalled(const FString& packageName) const;

	/**
	 * Starts an installed app with string extras ("package", "package:action" or
	 * "package:action:component"). With quitAfterLaunch, this app quits once the other one starts.
	 */
	UFUNCTION(BlueprintCallable, Category = "Apex|Launcher")
	bool LaunchApplication(const FString& packageName, const TArray<FString>& extraKeys,
						   const TArray<FString>& extraValues, bool quitAfterLaunch = true);

	/** Opens a URL in the app that handles it; with quitAfterLaunch, this app quits on Android. */
	UFUNCTION(BlueprintCallable, Category = "Apex|Launcher")
	bool LaunchUrl(const FString& url, bool quitAfterLaunch = false);

	/** Quits this app, removing its task on Android. */
	UFUNCTION(BlueprintCallable, Category = "Apex|Launcher")
	void QuitApplication();

	/** The launch intent's data URL on Android (e.g. pixovr://...?pixotoken=...), or "". */
	UFUNCTION(BlueprintPure, Category = "Apex|Launcher")
	FString GetIntentData() const;

	/** A launch intent extra on Android, or "". */
	UFUNCTION(BlueprintPure, Category = "Apex|Launcher")
	FString GetApplicationExtra(const FString& extraKey) const;

	// ------------------------------------------------------------ Installing

	/** The installed app's version name, or "" when it is not installed (always "" off Android). */
	UFUNCTION(BlueprintPure, Category = "Apex|Launcher")
	FString GetApplicationVersionName(const FString& packageName) const;

	/** Downloads an APK (or a zip holding it) from url and installs it. Progress: OnPackageEvent. */
	UFUNCTION(BlueprintCallable, Category = "Apex|Launcher", meta = (WorldContext = "worldContextObject"))
	bool InstallApplication(UObject* worldContextObject, const FString& packageName, const FString& url);

	/** Installs over an installed app: the same as InstallApplication. */
	UFUNCTION(BlueprintCallable, Category = "Apex|Launcher", meta = (WorldContext = "worldContextObject"))
	bool UpdateApplication(UObject* worldContextObject, const FString& packageName, const FString& url);

	UFUNCTION(BlueprintCallable, Category = "Apex|Launcher")
	bool UninstallApplication(const FString& packageName);

	/** True when the app is not installed, or its version is older than availableVersion. */
	UFUNCTION(BlueprintPure, Category = "Apex|Launcher")
	bool IsUpdateAvailable(const FString& packageName, const FString& availableVersion) const;

	/** Compares dotted versions ("1.4.10" > "1.4.9"): negative, 0 or positive, like strcmp. */
	UFUNCTION(BlueprintPure, Category = "Apex|Launcher")
	static int32 CompareVersions(const FString& firstVersion, const FString& secondVersion);

	/** Reports a package event as if Java had: for mock downloaders, and called by the JNI glue. */
	UFUNCTION(BlueprintCallable, Category = "Apex|Launcher")
	void ReportPackageEvent(const FString& packageName, EApexPackageEvent packageEvent, float progressPercent = 0.0f,
							int64 sizeBytes = 0);

	/** Off Android, InstallApplication spawns this actor class and calls OnMockDownloaderSpawned. */
	UFUNCTION(BlueprintCallable, Category = "Apex|Launcher|Debug")
	void SetMockDownloaderClass(TSubclassOf<AActor> mockDownloaderClass);

	UFUNCTION(BlueprintPure, Category = "Apex|Launcher|Debug")
	bool UsesMockDownloader() const;

	UFUNCTION(BlueprintCallable, Category = "Apex|Launcher|Debug")
	void LogInstalledPackages();

	// ------------------------------------------------------------ Shared files

	/** Text files in the shared Download folder (Android 10+), which outlive this app. */
	UFUNCTION(BlueprintPure, Category = "Apex|Launcher|Shared Files")
	bool DoesSharedFileExist(const FString& fileName) const;

	/** Returns the file's content URI, or "" on failure. */
	UFUNCTION(BlueprintCallable, Category = "Apex|Launcher|Shared Files")
	FString WriteSharedFile(const FString& fileName, const FString& fileContent);

	/** Returns "" when the file is missing. */
	UFUNCTION(BlueprintCallable, Category = "Apex|Launcher|Shared Files")
	FString ReadSharedFile(const FString& fileName) const;

	UFUNCTION(BlueprintCallable, Category = "Apex|Launcher|Shared Files")
	bool DeleteSharedFile(const FString& fileName);

	// ------------------------------------------------------------ Device

	/** The serial number ManageXR reports, else the system's, else "". ManageXR may report it late. */
	UFUNCTION(BlueprintPure, Category = "Apex|Launcher|Device")
	FString GetDeviceSerialNumber() const;

	UFUNCTION(BlueprintPure, Category = "Apex|Launcher|Device")
	FString GetDeviceMacAddress() const;

	/** True when ManageXR manages the headset. */
	UFUNCTION(BlueprintPure, Category = "Apex|Launcher|Device")
	bool IsDeviceManaged() const;

	UPROPERTY(BlueprintAssignable, Category = "Apex|Launcher")
	FApexPackageEventDelegate OnPackageEvent;

	FApexPackageEventNativeDelegate OnPackageEventNative;

	UPROPERTY(BlueprintAssignable, Category = "Apex|Launcher|Debug")
	FApexMockDownloaderSpawnedDelegate OnMockDownloaderSpawned;

private:
	UPROPERTY()
	TSubclassOf<AActor> m_mockDownloaderClass;
};
