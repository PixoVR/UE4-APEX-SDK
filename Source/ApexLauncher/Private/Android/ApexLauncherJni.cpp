// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

// Receives PixoEventNotifier's native calls (Java/src/com/PixoPlatform/PixoEventNotifier.java) and
// reports them on the game thread. Java calls these from the main looper, never the game thread.

#include "ApexLauncherSubsystem.h"

#include "Android/AndroidJNI.h"
#include "Android/AndroidJavaEnv.h"
#include "Async/Async.h"

namespace
{
void ReportOnGameThread(JNIEnv* javaEnvironment, jstring javaPackageName, EApexPackageEvent packageEvent,
						float progressPercent = 0.0f, int64 sizeBytes = 0)
{
	FString packageName = FJavaHelper::FStringFromParam(javaEnvironment, javaPackageName);
	AsyncTask(ENamedThreads::GameThread, [packageName, packageEvent, progressPercent, sizeBytes]()
	{
		if (UApexLauncherSubsystem* launcher = UApexLauncherSubsystem::Get())
		{
			launcher->ReportPackageEvent(packageName, packageEvent, progressPercent, sizeBytes);
		}
	});
}
}  // namespace

#define APEX_PACKAGE_EVENT_NATIVE(NativeName) \
	extern "C" JNIEXPORT void JNICALL Java_com_Pixo_PixoPlatform_PixoEventNotifier_##NativeName

APEX_PACKAGE_EVENT_NATIVE(nativePDownloadStartedCallback)
(JNIEnv* javaEnvironment, jobject, jstring packageName, jint downloadSizeBytes)
{
	ReportOnGameThread(javaEnvironment, packageName, EApexPackageEvent::DOWNLOAD_STARTED, 0.0f, downloadSizeBytes);
}

APEX_PACKAGE_EVENT_NATIVE(nativePDownloadCompleteCallback)(JNIEnv* javaEnvironment, jobject, jstring packageName)
{
	ReportOnGameThread(javaEnvironment, packageName, EApexPackageEvent::DOWNLOAD_COMPLETED, 100.0f);
}

APEX_PACKAGE_EVENT_NATIVE(nativePDownloadProgressCallback)
(JNIEnv* javaEnvironment, jobject, jstring packageName, jfloat progressPercent)
{
	ReportOnGameThread(javaEnvironment, packageName, EApexPackageEvent::DOWNLOAD_PROGRESSED, progressPercent);
}

APEX_PACKAGE_EVENT_NATIVE(nativePDownloadCanceledCallback)(JNIEnv* javaEnvironment, jobject, jstring packageName)
{
	ReportOnGameThread(javaEnvironment, packageName, EApexPackageEvent::DOWNLOAD_CANCELED);
}

APEX_PACKAGE_EVENT_NATIVE(nativePDownloadFailedCallback)(JNIEnv* javaEnvironment, jobject, jstring packageName)
{
	ReportOnGameThread(javaEnvironment, packageName, EApexPackageEvent::DOWNLOAD_FAILED);
}

APEX_PACKAGE_EVENT_NATIVE(nativePExtractionStartedCallback)(JNIEnv* javaEnvironment, jobject, jstring packageName)
{
	ReportOnGameThread(javaEnvironment, packageName, EApexPackageEvent::EXTRACTION_STARTED);
}

APEX_PACKAGE_EVENT_NATIVE(nativePExtractionCompletedCallback)(JNIEnv* javaEnvironment, jobject, jstring packageName)
{
	ReportOnGameThread(javaEnvironment, packageName, EApexPackageEvent::EXTRACTION_COMPLETED, 100.0f);
}

APEX_PACKAGE_EVENT_NATIVE(nativePExtractionProgressCallback)
(JNIEnv* javaEnvironment, jobject, jstring packageName, jfloat progressPercent)
{
	ReportOnGameThread(javaEnvironment, packageName, EApexPackageEvent::EXTRACTION_PROGRESSED, progressPercent);
}

APEX_PACKAGE_EVENT_NATIVE(nativePInstallReadyCallback)
(JNIEnv* javaEnvironment, jobject, jstring packageName, jlong installSizeBytes)
{
	ReportOnGameThread(javaEnvironment, packageName, EApexPackageEvent::INSTALL_READY, 0.0f, installSizeBytes);
}

APEX_PACKAGE_EVENT_NATIVE(nativePInstallationStartedCallback)(JNIEnv* javaEnvironment, jobject, jstring packageName)
{
	ReportOnGameThread(javaEnvironment, packageName, EApexPackageEvent::INSTALLATION_STARTED);
}

APEX_PACKAGE_EVENT_NATIVE(nativePInstallationCompletedCallback)(JNIEnv* javaEnvironment, jobject, jstring packageName)
{
	ReportOnGameThread(javaEnvironment, packageName, EApexPackageEvent::INSTALLATION_COMPLETED, 100.0f);
}

APEX_PACKAGE_EVENT_NATIVE(nativePInstallationCanceledCallback)(JNIEnv* javaEnvironment, jobject, jstring packageName)
{
	ReportOnGameThread(javaEnvironment, packageName, EApexPackageEvent::INSTALLATION_CANCELED);
}

APEX_PACKAGE_EVENT_NATIVE(nativePInstallationFailedCallback)(JNIEnv* javaEnvironment, jobject, jstring packageName)
{
	ReportOnGameThread(javaEnvironment, packageName, EApexPackageEvent::INSTALLATION_FAILED);
}

APEX_PACKAGE_EVENT_NATIVE(nativePInstallationProgressedCallback)
(JNIEnv* javaEnvironment, jobject, jstring packageName, jfloat progressPercent)
{
	ReportOnGameThread(javaEnvironment, packageName, EApexPackageEvent::INSTALLATION_PROGRESSED, progressPercent);
}

APEX_PACKAGE_EVENT_NATIVE(nativePUninstallationStartedCallback)(JNIEnv* javaEnvironment, jobject, jstring packageName)
{
	ReportOnGameThread(javaEnvironment, packageName, EApexPackageEvent::UNINSTALLATION_STARTED);
}

APEX_PACKAGE_EVENT_NATIVE(nativePUninstallationCompletedCallback)(JNIEnv* javaEnvironment, jobject, jstring packageName)
{
	ReportOnGameThread(javaEnvironment, packageName, EApexPackageEvent::UNINSTALLATION_COMPLETED, 100.0f);
}

APEX_PACKAGE_EVENT_NATIVE(nativePUninstallationCanceledCallback)(JNIEnv* javaEnvironment, jobject, jstring packageName)
{
	ReportOnGameThread(javaEnvironment, packageName, EApexPackageEvent::UNINSTALLATION_CANCELED);
}

APEX_PACKAGE_EVENT_NATIVE(nativePUninstallationFailedCallback)(JNIEnv* javaEnvironment, jobject, jstring packageName)
{
	ReportOnGameThread(javaEnvironment, packageName, EApexPackageEvent::UNINSTALLATION_FAILED);
}

#undef APEX_PACKAGE_EVENT_NATIVE
