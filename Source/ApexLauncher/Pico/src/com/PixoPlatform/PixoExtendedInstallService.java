package com.Pixo.PixoPlatform;

import android.util.Log;
import android.content.Context;
import android.os.RemoteException;
import android.content.Intent;
import com.pvr.tobservice.ToBServiceHelper;
import com.pvr.tobservice.enums.PBS_PackageControlEnum;
import com.pvr.tobservice.interfaces.IIntCallback;

public class PixoExtendedInstallService extends PixoInstallerService {

    public PixoExtendedInstallService() {
		super("PixoExtendedInstallService");
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
        Log.d(TAG, "Handling Pico install service intent.");
        super.onHandleIntent(InIntent);

        switch(InstallActionType)
        {
            case 0:
                Install(PackageName, APKPackage);
                break;
            case 1:
                Uninstall(PackageName);
                break;
                
        }
    }

    @Override
    public void Install(String packageName, PixoItemPackage apkPackage) {
        Log.d(TAG, "PixoExtendedInstallService - Installing " + packageName + " from path " + apkPackage.ApkFile);

        PackageName = packageName;
        
        PixoEventNotifier.getInstance().InstallationStarted(PackageName);
        try {
            ToBServiceHelper.getInstance().getServiceBinder().pbsControlAPPManger(
                PBS_PackageControlEnum.PACKAGE_SILENCE_INSTALL, apkPackage.ApkFile, 0, new IIntCallback.Stub() {
                    @Override
                    public void callback(int result) throws RemoteException {
                        Log.e(TAG, "ToBServiceHelper - Install Callback: "+result);
        
                        if(result == 0) {
                            Log.e(TAG, "Install completed. With " + apkPackage.ItemFiles.size() + " files.");
                            Log.e(TAG, "Checking member file count with " + APKPackage.ItemFiles.size() + " files.");
                            if((apkPackage.ObbFile != null && !apkPackage.ObbFile.trim().isEmpty()) || 
                                apkPackage.ItemFiles.size() >= 2) {
                                PixoAsyncMoveFile MoveFileTask = new PixoAsyncMoveFile(AppContext);
                                MoveFileTask.execute(apkPackage);
                            }
                            else
                            {
                                PixoEventNotifier.getInstance().InstallationCompleted(PackageName);
                            }
                        }
                        else {
                            Log.e(TAG, "Install failed.");
                            PixoEventNotifier.getInstance().InstallationFailed(PackageName);
                        }
                    }
            });
        }
        catch(Exception e) {
            Log.e(TAG, "ToBServiceHelper install failed." + e);
            PixoEventNotifier.getInstance().InstallationFailed(PackageName);
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
        Log.i(TAG, "PixoExtendedInstallService - Uninstall " + packageName);
    
        PackageName = packageName;
        
        PixoEventNotifier.getInstance().UninstallationStarted(PackageName);
        try {
            ToBServiceHelper.getInstance().getServiceBinder().pbsControlAPPManger(
                PBS_PackageControlEnum.PACKAGE_SILENCE_UNINSTALL, PackageName, 0, new IIntCallback.Stub() {
                    @Override
                    public void callback(int result) throws RemoteException {
                        Log.e(TAG, "ToBServiceHelper - Uninstall Callback: "+result);
        
                        if(result == 0) {
                            Log.e(TAG, "Uninstall completed.");
                            PixoEventNotifier.getInstance().UninstallationCompleted(PackageName);
                        }
                        else {
                            Log.e(TAG, "Uninstall failed.");
                            PixoEventNotifier.getInstance().UninstallationFailed(PackageName);
                        }
                    }
            });
        }
        catch(Exception e) {
            Log.e(TAG, "ToBServiceHelper uninstall failed." + e);
            PixoEventNotifier.getInstance().UninstallationFailed(PackageName);
        }
    }
}