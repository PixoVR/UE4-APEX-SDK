package com.Pixo.PixoPlatform;

import android.util.Log;

public class PixoEventNotifier {
    protected static final String TAG = "PixoEventNotifier";
    protected static PixoEventNotifier Instance = null;

    public static PixoEventNotifier getInstance() {
        if(Instance == null) {
            Instance = new PixoEventNotifier();
        }

        return Instance;
    }

    // Downloads
    public native void nativePDownloadStartedCallback(String PackageName, int DownloadSize);
    public native void nativePDownloadCompleteCallback(String PackageName);
    public native void nativePDownloadProgressCallback(String PackageName, float ProgressPercentage);
    public native void nativePDownloadCanceledCallback(String PackageName);
    public native void nativePDownloadFailedCallback(String PackageName);

    public void DownloadStarted(String PackageName, int DownloadSize) {
        Log.d(TAG, "Download Started.");
        nativePDownloadStartedCallback(PackageName, DownloadSize);
    }

    public void DownloadCompleted(String PackageName) {
        Log.d(TAG, "Download Completed.");
        nativePDownloadCompleteCallback(PackageName);
    }

    public void DownloadProgressUpdate(String PackageName, float ProgressPercentage) {
        Log.d(TAG, "Download Progressed.");
        nativePDownloadProgressCallback(PackageName, ProgressPercentage);
    }

    public void DownloadCanceled(String PackageName) {
        nativePDownloadCanceledCallback(PackageName);
    }

    public void DownloadFailed(String PackageName) {
        nativePDownloadFailedCallback(PackageName);
    }

    // Extractions
    public native void nativePExtractionStartedCallback(String PackageName);
    public native void nativePExtractionCompletedCallback(String PackageName);
    public native void nativePExtractionProgressCallback(String PackageName, float ProgressPercentage);

    public void ExtractionStarted(String PackageName) {
        nativePExtractionStartedCallback(PackageName);
    }

    public void ExtractionCompleted(String PackageName) {
        nativePExtractionCompletedCallback(PackageName);
    }

    public void ExtractionProgressUpdate(String PackageName, float ProgressPercentage) {
        nativePExtractionProgressCallback(PackageName, ProgressPercentage);
    }

    // Other Notification Types
    public native void nativePInstallReadyCallback(String PackageName, long InstallSize);

    public void InstallReady(PixoItemPackage Item) {
        nativePInstallReadyCallback(Item.PackageName, Item.getSize());
    }


    // Installation
    public native void nativePInstallationStartedCallback(String PackageName);
    public native void nativePInstallationCompletedCallback(String PackageName);
    public native void nativePInstallationCanceledCallback(String PackageName);
    public native void nativePInstallationFailedCallback(String PackageName);
    public native void nativePInstallationProgressedCallback(String PackageName, float ProgressPercentage);

    public void InstallationStarted(String PackageName) {
        Log.e(TAG, "Installation started.");
        nativePInstallationStartedCallback(PackageName);
    }

    public void InstallationCompleted(String PackageName) {
        Log.e(TAG, "Installation completed.");
        nativePInstallationCompletedCallback(PackageName);
    }

    public void InstallationCanceled(String PackageName) {
        Log.e(TAG, "Installation canceled.");
        nativePInstallationCanceledCallback(PackageName);
    }

    public void InstallationFailed(String PackageName) {
        Log.e(TAG, "Installation failed.");
        nativePInstallationFailedCallback(PackageName);
    }

	public void InstallationProgressed(String PackageName, float ProgressPercentage)
    {
        nativePInstallationProgressedCallback(PackageName, ProgressPercentage);
        Log.e(TAG, "Install for " + PackageName + " at " + ProgressPercentage + " percent completed.");
    }


    // Uninstall
    public native void nativePUninstallationStartedCallback(String PackageName);
    public native void nativePUninstallationCompletedCallback(String PackageName);
    public native void nativePUninstallationFailedCallback(String PackageName);
    public native void nativePUninstallationCanceledCallback(String PackageName);

    public void UninstallationStarted(String PackageName) {
        nativePUninstallationStartedCallback(PackageName);
    }

    public void UninstallationCompleted(String PackageName) {
        nativePUninstallationCompletedCallback(PackageName);
    }

    public void UninstallationCanceled(String PackageName) {
        nativePUninstallationCanceledCallback(PackageName);
    }

    public void UninstallationFailed(String PackageName) {
        nativePUninstallationFailedCallback(PackageName);
    }
}
