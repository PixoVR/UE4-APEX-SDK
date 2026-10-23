package com.Pixo.PixoPlatform;

import com.htc.customizedlib.ConfigPackManager;
import com.htc.customizedlib.CustomizedService;
import com.htc.customizedlib.AppManager;
import android.util.Log;
import android.content.Context;
import android.os.Environment;
import android.os.ResultReceiver;
import android.app.IntentService;
import android.content.Intent;
import android.os.Bundle;
import java.io.File;

public class PixoExtendedInstallService extends PixoInstallerService {

    private AppManager.InstallAppListener mInstallAppListener = new AppManager.InstallAppListener() {
        @Override
        public void onSuccess(String appPath) {
            Log.e(TAG, "onSuccess, appPath:" + appPath);

            Log.e(TAG, "Checking member file count with " + APKPackage.ItemFiles.size() + " files.");
            if((APKPackage.ObbFile != null && !APKPackage.ObbFile.trim().isEmpty()) || 
                APKPackage.ItemFiles.size() >= 2) {
                Log.e(TAG, "Attempting to move obb file.");
                PixoAsyncMoveFile MoveFileTask = new PixoAsyncMoveFile(AppContext);
                MoveFileTask.execute(APKPackage);
            }
            else {
                // The download manager follows installs by package name, not the APK path.
                PixoEventNotifier.getInstance().InstallationCompleted(APKPackage.PackageName);
            }
        }
    
        @Override
        public void onFail(String appPath, int errorCode, String errorMessage) {
            Log.e(TAG, "onFail, appPath:" + appPath + ", errorCode = " + errorCode + ", errorMessage = " + errorMessage);
            PixoEventNotifier.getInstance().InstallationFailed(APKPackage.PackageName);
        }
    };

    private AppManager.UninstallAppListener mUninstallAppListener = new AppManager.UninstallAppListener() {
        @Override
        public void onSuccess(String packageName) {
            Log.e(TAG, "onSuccess, packageName:" + packageName);
            PixoEventNotifier.getInstance().UninstallationCompleted(packageName);
        }

        @Override
        public void onFail(String packageName, int errorCode, String errorMessage) {
            Log.e(TAG, "onFail, packageName:" + packageName + ", errorCode = " + errorCode + ", errorMessage = " +
            errorMessage);
            PixoEventNotifier.getInstance().UninstallationFailed(packageName);
        }
    };

    public PixoExtendedInstallService() {
		super("PixoExtendedInstallService");

        if(AppContext == null)
        {
            AppContext = this;
        }

        if(AppContext == null)
        {
            Log.e(TAG, "Context is null.");
        }
        else
        {
            Log.e(TAG, "Context is not null.");
        }
	}

	public PixoExtendedInstallService(String name) {
		super(name);
	}

    @Override
	public void onDestroy() {
		super.onDestroy();
		CancelInstall = true;
	}

	@Override
	protected void onHandleIntent(Intent InIntent) {
        super.onHandleIntent(InIntent);
        Log.d(TAG, "Handling HTC PPlatform install service intent.");


        switch(InstallActionType)
        {
            case 0:
                Log.d(TAG, "OnHandleIntent PixoIntallerService - Installing " + PackageName + " from path " + APKPackage.ApkFile);
                Install(PackageName, APKPackage);
                break;
            case 1:
                Log.d(TAG, "OnHandleIntent PixoIntallerService - Uninstalling " + PackageName);
                Uninstall(PackageName);
                break;
        }
    }

    @Override
    protected void Setup() {
        Log.d(TAG, "HTC PixoExtendedInstallService Setup.");

        if(AppContext == null)
        {
            Log.d(TAG, "Getting app context.");
            AppContext = getApplicationContext();
        }

        IsSetup = true;
    }

    @Override
    public void Install(String packageName, PixoItemPackage apkPackage) {
        Log.i(TAG, "PixoIntallerService - Installing " + packageName + " from path " + apkPackage.ApkFile);

        if(IsSetup == true)
        {
            Log.d(TAG, "Setup is true.");
            PixoEventNotifier.getInstance().InstallationStarted(packageName);
            try {
                Log.d(TAG, "Using customized service to install app.");
                CustomizedService.installApp(apkPackage.ApkFile, true, mInstallAppListener);
            }
            catch(Exception e) {
                Log.e(TAG, "Exception trying to install " + packageName + " with error " + e);
                PixoEventNotifier.getInstance().InstallationFailed(packageName);
            }
        }
        else
        {
            Log.d(TAG, "No valid setup.");
            PixoEventNotifier.getInstance().InstallationCanceled(packageName);
        }
    }

    // This code is neccessary to force proper Java compilation.
    public static void TestThreadFunction() {
        new Thread(()-> {
            Log.i(TAG, "test lambda expression");
        }).start();
    }

    @Override
    public void Uninstall(String packageName) {
        Log.i(TAG, "PixoIntallerService - Uninstall " + packageName);

        PixoEventNotifier.getInstance().UninstallationStarted(packageName);
        try {
            CustomizedService.uninstallApp(packageName, mUninstallAppListener);
        }
        catch(Exception e) {
            Log.e(TAG, "Exception trying to uninstall " + packageName + " with error " + e);
            PixoEventNotifier.getInstance().UninstallationFailed(packageName);
        }
    }
}