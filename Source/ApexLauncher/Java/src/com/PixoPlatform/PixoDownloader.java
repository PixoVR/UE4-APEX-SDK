package com.Pixo.PixoPlatform;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.ResultReceiver;
import android.util.Log;

public class PixoDownloader extends ResultReceiver {
    protected static final String TAG = "PPlatform";
    
    Context AppContext;
    String DownloadUrl;
    String PackageName;
    int HeadsetType;
    PixoEventNotifier EventNotifier;
    private Intent ServiceIntent = null;

    public PixoDownloader(Handler handler) {
        super(handler);
    }

    public static PixoDownloader createDownloader(Context InAppContext, String InDownloadUrl, String InPackageName, int InHeadsetType) {
        Log.d(TAG, "Creating a downloader with url " + InDownloadUrl + ".");
        Handler handler = new Handler(Looper.getMainLooper());
        PixoDownloader DownloaderInstance = new PixoDownloader(handler);
        DownloaderInstance.AppContext = InAppContext;
        DownloaderInstance.DownloadUrl = InDownloadUrl;
        DownloaderInstance.PackageName = InPackageName;
        DownloaderInstance.EventNotifier = PixoEventNotifier.getInstance();
        DownloaderInstance.HeadsetType = InHeadsetType;
        return DownloaderInstance;
    }

    public void startDownloadPackage() {
        Log.d(TAG, "StartDownloadPackage for URL " + DownloadUrl + " with APK " + PackageName);
        ServiceIntent = new Intent(AppContext, PixoDownloaderService.class);
        ServiceIntent.putExtra(PixoDownloaderService.DOWNLOAD_URL, DownloadUrl);
        ServiceIntent.putExtra(PixoDownloaderService.DOWNLOAD_NAME, PackageName);
        ServiceIntent.putExtra(PixoDownloaderService.DOWNLOADER_RECEIVER, this);
        Log.d(TAG, "Starting download service intent.");
        AppContext.startService(ServiceIntent);
    }

    public void cancelDownloadPackage() {
        AppContext.stopService(ServiceIntent);
    }

    @Override
    protected void onReceiveResult(int ResultCode, Bundle ResultData) {
        super.onReceiveResult(ResultCode, ResultData);

        if (EventNotifier == null) {
            return;
        }

        if (ResultCode == PixoDownloaderService.STATUS_OK)
        {
            String apkName = ResultData.getString(PixoDownloaderService.DOWNLOAD_NAME);
            Log.d(TAG, "Apk: " + apkName);
            if (ResultData.containsKey(PixoDownloaderService.DOWNLOAD_STARTED)
                && ResultData.getBoolean(PixoDownloaderService.DOWNLOAD_STARTED))
            {
                Log.d(TAG, "Download started event.");
                int downloadSize = ResultData.getInt(PixoDownloaderService.DOWNLOAD_SIZE);
                EventNotifier.DownloadStarted(apkName, downloadSize);
            } 
            else if (ResultData.containsKey(PixoDownloaderService.DOWNLOAD_COMPLETED)
                && ResultData.getBoolean(PixoDownloaderService.DOWNLOAD_COMPLETED))
            {
                Log.d(TAG, "Download completed event.");
                EventNotifier.DownloadCompleted(apkName);
            } 
            else if (ResultData.containsKey(PixoDownloaderService.DOWNLOAD_PROGRESS))
            {
                Log.d(TAG, "Download progress event.");
                int progress = ResultData.getInt(PixoDownloaderService.DOWNLOAD_PROGRESS);
                EventNotifier.DownloadProgressUpdate(apkName, progress);
            } 
            else if (ResultData.containsKey(PixoDownloaderService.DOWNLOAD_EXTRACTION_STARTED))
            {
                Log.d(TAG, "Download extraction started event.");
                EventNotifier.ExtractionProgressUpdate(apkName, 0);
                EventNotifier.ExtractionStarted(apkName);
            }
            else if (ResultData.containsKey(PixoDownloaderService.DOWNLOAD_EXTRACTION_PROGRESS))
            {
                int progress = ResultData.getInt(PixoDownloaderService.DOWNLOAD_EXTRACTION_PROGRESS);
                Log.d(TAG, "Download extraction progress event at " + progress);
                EventNotifier.ExtractionProgressUpdate(apkName, progress);
            }
            else if (ResultData.containsKey(PixoDownloaderService.DOWNLOAD_EXTRACTION_COMPLTED))
            {
                Log.d(TAG, "Download extraction completed event.");
                EventNotifier.ExtractionCompleted(apkName);
            } 
            else if (ResultData.containsKey(PixoDownloaderService.DOWNLOADER_READY_TO_INSTALL))
            {
                Log.d(TAG, "Download ready to install event.");
                PixoItemPackage Item = ResultData.getParcelable(PixoDownloaderService.DOWNLOAD_PACKAGE);
                EventNotifier.InstallReady(Item);
                Log.d(TAG, "Creating installer with context " + AppContext);
                PixoInstaller APKInstaller = PixoInstaller.createInstaller(AppContext, Item.PackageName, Item, HeadsetType);
                APKInstaller.startInstallPackage();
            }
        }
        else if (ResultCode == PixoDownloaderService.STATUS_FAILED)
        {
            String apkName = ResultData.getString(PixoDownloaderService.DOWNLOAD_NAME);
            if (ResultData.containsKey(PixoDownloaderService.DOWNLOAD_CANCELED))
            {
                EventNotifier.DownloadCanceled(apkName);
            }
            else
            {
                EventNotifier.DownloadFailed(apkName);
            }
        }
    }
}
