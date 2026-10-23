package com.Pixo.PixoPlatform;

import android.app.DownloadManager;
import android.app.IntentService;
import android.content.Context;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Environment;
import android.os.ResultReceiver;
import android.util.Log;

import java.io.IOException;

public class PixoAsyncExtractor extends AsyncTask<String, Integer, Integer> {
    protected static final String TAG = "PVRM";
    Context AppContext = null;
	String ApkName = null;
    DownloadManager DownloadManagerService = null;
    PixoDownloaderService DownloaderService = null;
    ResultReceiver DownloadResultReceiver = null;
    String DestinationDirectory;
    String OutputFilePath;
    PixoItemPackage StorePackage = null;

    public PixoAsyncExtractor(Context InAppContext, DownloadManager InDownloadManagerService, PixoDownloaderService InDownloaderService, ResultReceiver InDownloadResultReceiver, String InDestinationDirectory, String InOutputFilePath) {
        AppContext = InAppContext;
        DownloadManagerService = InDownloadManagerService;
        DownloaderService = InDownloaderService;
        DownloadResultReceiver = InDownloadResultReceiver;
        DestinationDirectory = InDestinationDirectory;
        OutputFilePath = InOutputFilePath;
    }

    @Override
    protected Integer doInBackground(String... ApkNames) {
		this.ApkName = ApkNames[0];
        try {
            Log.e(TAG, "Extraction started on file " + OutputFilePath + "!");

            DownloaderService.extractionStarted(DownloadResultReceiver, ApkName);

            Log.e(TAG, "Unzipping package.");

            StorePackage = DownloaderService.unzipPackage(DownloadResultReceiver, DestinationDirectory, OutputFilePath, ApkName);


            DownloaderService.sendExtractionProgress(100, DownloadResultReceiver, ApkName);

            return 0;
        } catch (IOException e) {
            PixoEventNotifier.getInstance().DownloadFailed(ApkName);
            e.printStackTrace();
        }
        return 1;
    }

    @Override
    protected void onPreExecute() {
        super.onPreExecute();
    }

    @Override
    protected void onPostExecute(Integer b) {
        super.onPostExecute(b);

        Log.e(TAG, "Extraction completed.");
        DownloaderService.extractionCompleted(DownloadResultReceiver, ApkName);

        DownloaderService.readyToInstall(DownloadResultReceiver, ApkName, StorePackage);
    }

    @Override
    protected void onProgressUpdate(Integer... progress)
    {
        // Will be fired when publishProgress() is called.
    }
}
