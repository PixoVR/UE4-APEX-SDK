package com.Pixo.PixoPlatform;

import android.util.Log;
import android.content.Context;
import android.os.Environment;
import android.os.ResultReceiver;
import android.app.IntentService;
import android.content.Intent;
import android.os.Bundle;
import java.io.File;

public class PixoInstallerService extends IntentService implements PixoInstallerInterface {

    protected static String TAG = "PixoInstallerService";

    // Bundle item strings
    public static final String INSTALL_ACTION = "install_action";
    public static final String INSTALL_PACKAGE = "install_url";
	public static final String INSTALL_NAME = "install_name";
	public static final String INSTALL_RECEIVER = "install_receiver";
    public static final String INSTALL_APPCONTEXT = "install_app_context";

	// Status strings
	public static final String INSTALL_STARTED = "install_started";
	public static final String INSTALL_COMPLETED = "install_completed";
	public static final String INSTALL_CANCELED = "install_canceled";
	public static final String INSTALL_FAILED = "install_failed";
	public static final String INSTALL_PROGRESS = "install_progress";

    public static int STATUS_OK = 222;
	public static int STATUS_FAILED = 333;

    public byte InstallActionType;
    public boolean CancelInstall;
    ResultReceiver InstallResultReceiver;
    public String PackageName;
    public PixoItemPackage APKPackage;
    public Context AppContext;

    public static boolean IsSetup = false;

    public PixoInstallerService() {
		super("PixoInstallerService");
	}

	public PixoInstallerService(String name) {
		super(name);
	}

    @Override
	public void onDestroy() {
		super.onDestroy();
		CancelInstall = true;
	}

	@Override
	protected void onHandleIntent(Intent InIntent) {
        Log.d(TAG, "Handling PPlatform install service intent.");
		Bundle IntentBundle = InIntent.getExtras();
		if (null == IntentBundle) {
			Log.w(TAG, "Missing intent bundle.");		
			return;
		}

		if (!IntentBundle.containsKey(INSTALL_PACKAGE)) {
			Log.w(TAG, "Intent bundle is missing install apk package.");
		}

        Log.d(TAG, "Getting parcelable receiver.");
		InstallResultReceiver = IntentBundle.getParcelable(INSTALL_RECEIVER);
        Log.d(TAG, "Getting Install Action Type");
        InstallActionType = IntentBundle.getByte(INSTALL_ACTION, (byte)0);
        Log.d(TAG, "Getting package name.");
		PackageName = IntentBundle.getString(INSTALL_NAME);
        Log.d(TAG, "Getting install package.");
        APKPackage = IntentBundle.getParcelable(INSTALL_PACKAGE);
        Log.d(TAG, "Getting current installer instance.");
        PixoInstaller InstallReceiver = PixoInstaller.GetCurrentInstallerInstance();
        if(InstallReceiver != null)
        {
            Log.d(TAG, "Installer receiver was valid, getting app context.");
            AppContext = InstallReceiver.AppContext;
        }
        else
        {
            Log.d(TAG, "Installer receiver was invalid, falling back to getApplicationContext.");
            AppContext = getApplicationContext();
        }

        Log.d(TAG, "Calling setup!");
        Setup();
    }

    protected void Setup() {
        Log.d(TAG, "Setting up app context in Installer service.");
    }

    @Override
    public void Install(String packageName, PixoItemPackage apkPackage) {
        Log.i(TAG, "PixoIntallerService - Installing " + packageName + " from path " + apkPackage.ApkFile);

        PixoEventNotifier.getInstance().InstallationStarted(packageName);
        try {
            //CustomizedService.installApp(apkPackage.ApkFile, true, mInstallAppListener);
            PixoEventNotifier.getInstance().InstallationCompleted(packageName);
        }
        catch(Exception e) {
            Log.e(TAG, "Exception trying to install " + packageName + " with error " + e);
            PixoEventNotifier.getInstance().InstallationFailed(packageName);
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
            PixoEventNotifier.getInstance().UninstallationCompleted(packageName);        
        }
        catch(Exception e) {
            Log.e(TAG, "Exception trying to uninstall " + packageName + " with error " + e);
            PixoEventNotifier.getInstance().UninstallationFailed(packageName);
        }
    }
}