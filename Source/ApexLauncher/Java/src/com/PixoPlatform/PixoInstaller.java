package com.Pixo.PixoPlatform;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.ResultReceiver;
import android.util.Log;

public class PixoInstaller extends ResultReceiver {
    protected static final String TAG = "PPlatform";
    
    public Context AppContext;
    String APKName;
    PixoItemPackage APKPackage; 
    int HeadsetType;
    PixoEventNotifier EventNotifier;
    private Intent ServiceIntent = null;

    static PixoInstaller CurrentInstallerInstance = null;

    public PixoInstaller(Handler handler) {
        super(handler);
    }

    public static PixoInstaller createInstaller(Context InAppContext, String InAPKName, PixoItemPackage InAPKPackage, int InHeadsetType) {
        Log.d(TAG, "Creating a installer with apk " + InAPKName + ".");
        Handler handler = new Handler(Looper.getMainLooper());
        PixoInstaller InstallerInstance = new PixoInstaller(handler);
        InstallerInstance.AppContext = InAppContext;
        InstallerInstance.APKName = InAPKName;
        InstallerInstance.APKPackage = InAPKPackage;
        InstallerInstance.HeadsetType = InHeadsetType;
        InstallerInstance.EventNotifier = PixoEventNotifier.getInstance();
        CurrentInstallerInstance = InstallerInstance;
        return InstallerInstance;
    }

    public static PixoInstaller GetCurrentInstallerInstance() {
        return CurrentInstallerInstance;
    }

    public void startInstallPackage() {
        Log.d(TAG, "StartInstallPackage for apk " + APKName);
        
        ServiceIntent = new Intent(AppContext, PixoHelper.GetInstallerClass());

        ServiceIntent.putExtra(PixoInstallerService.INSTALL_NAME, APKName);
        ServiceIntent.putExtra(PixoInstallerService.INSTALL_PACKAGE, APKPackage);
        ServiceIntent.putExtra(PixoInstallerService.INSTALL_RECEIVER, this);
        Log.d(TAG, "Starting install service intent with context " + AppContext);
        AppContext.startService(ServiceIntent);
    }

    public void startUninstallPackage() {
        Log.d(TAG, "StartUninstallPackage for apk " + APKName);
        
        ServiceIntent = new Intent(AppContext, PixoHelper.GetInstallerClass());

        ServiceIntent.putExtra(PixoInstallerService.INSTALL_ACTION, (byte)1);
        ServiceIntent.putExtra(PixoInstallerService.INSTALL_NAME, APKName);
        ServiceIntent.putExtra(PixoInstallerService.INSTALL_RECEIVER, this);
        Log.d(TAG, "Starting uninstall service intent.");
        AppContext.startService(ServiceIntent);
    }

    public void cancelInstallPackage() {
        AppContext.stopService(ServiceIntent);
    }

	

    @Override
    protected void onReceiveResult(int ResultCode, Bundle ResultData) {
        super.onReceiveResult(ResultCode, ResultData);

        if (EventNotifier == null) {
            return;
        }

        if (ResultCode == PixoInstallerService.STATUS_OK)
        {
            if (ResultData.containsKey(PixoInstallerService.INSTALL_STARTED)
                && ResultData.getBoolean(PixoInstallerService.INSTALL_STARTED))
            {
                String InstallName = ResultData.getString(PixoInstallerService.INSTALL_NAME);
                EventNotifier.InstallationStarted(InstallName);
            } 
            else if (ResultData.containsKey(PixoInstallerService.INSTALL_COMPLETED)
                && ResultData.getBoolean(PixoInstallerService.INSTALL_COMPLETED))
            {
                String InstallName = ResultData.getString(PixoInstallerService.INSTALL_NAME);
                EventNotifier.InstallationCompleted(InstallName);
            } 
            else if (ResultData.containsKey(PixoInstallerService.INSTALL_PROGRESS))
            {
                int Progress = ResultData.getInt(PixoInstallerService.INSTALL_PROGRESS);
                String InstallName = ResultData.getString(PixoInstallerService.INSTALL_NAME);
                EventNotifier.InstallationProgressed(InstallName, Progress);
            } 
        }
        else if (ResultCode == PixoInstallerService.STATUS_FAILED)
        {
            String InstallName = ResultData.getString(PixoInstallerService.INSTALL_NAME);
            if (ResultData.containsKey(PixoInstallerService.INSTALL_CANCELED))
            {
                EventNotifier.InstallationCanceled(InstallName);
            }
            else
            {
                EventNotifier.InstallationFailed(InstallName);
            }
        }
    }
}
