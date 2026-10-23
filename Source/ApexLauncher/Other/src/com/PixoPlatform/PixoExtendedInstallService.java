package com.Pixo.PixoPlatform;

import android.util.Log;
import android.content.Context;
import android.os.Environment;
import android.os.ResultReceiver;
import android.app.IntentService;
import android.app.PendingIntent;
import android.content.Intent;
import android.content.IntentSender;
import android.content.pm.PackageInstaller;
import android.os.Bundle;
import android.net.Uri;
import java.io.File;
import java.io.InputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.lang.RuntimeException;

public class PixoExtendedInstallService extends PixoInstallerService {
    private static final String PACKAGE_INSTALLED_ACTION = "com.example.android.apis.content.SESSION_API_PACKAGE_INSTALLED";
    protected PackageInstaller.Session InstallerSession;

    public PixoExtendedInstallService() {
		super("PixoExtendedInstallService");
        Setup();
	}

	public PixoExtendedInstallService(String name) {
		super(name);
        Setup();
	}

    @Override
	public void onDestroy() {
		super.onDestroy();
		CancelInstall = true;
	}

	@Override
	protected void onHandleIntent(Intent InIntent) {
        Log.e("TAG", "PixoExtendedInstallService - Other device - Handling install service intent.");
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
    protected void Setup() {
    }

    @Override
    public void Install(String packageName, PixoItemPackage apkPackage) {
        Log.d(TAG, "PixoExtendedInstallService - Other device - Installing " + packageName + " from path " + apkPackage.ApkFile);

        boolean bHasFailed = false;
        PackageName = packageName;

        PixoEventNotifier.getInstance().InstallationStarted(PackageName);
        try {
            Log.d(TAG, "Getting package installer.");
            PackageInstaller packageInstaller = getPackageManager().getPackageInstaller();
            Log.d(TAG, "Setting session params.");
            PackageInstaller.SessionParams params = new PackageInstaller.SessionParams(
                    PackageInstaller.SessionParams.MODE_FULL_INSTALL);
            
            Log.d(TAG, "Creating session with params.");
            int sessionId = packageInstaller.createSession(params);
            
            Log.d(TAG, "Opening session " + sessionId);
            InstallerSession = packageInstaller.openSession(sessionId);
            
            Log.d(TAG, "Adding APK to install session.");
            addApkToInstallSession(apkPackage.ApkFile, InstallerSession);
            
            Log.d(TAG, "Creating package install intent.");
            // Create an install status receiver.
            Intent intent = new Intent(PACKAGE_INSTALLED_ACTION);
            Log.d(TAG, "Creating output file.");
            File outputFile = new File(apkPackage.ApkFile);
            Log.d(TAG, "Setting data on intent.");
            intent.setData(Uri.fromFile(outputFile));
            Log.d(TAG, "Setting grant read uri permission flag.");
            intent.setFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            Log.d(TAG, "Starting activity with intent.");
            AppContext.startActivity(intent);
            Log.d(TAG, "Activity started.");
            intent.setAction(PACKAGE_INSTALLED_ACTION);
            
            Log.d(TAG, "Creating pending intent.");
            PendingIntent pendingIntent = PendingIntent.getActivity(AppContext, 0, intent, 0);
            
            Log.d(TAG, "Getting status receiver.");
            IntentSender statusReceiver = pendingIntent.getIntentSender();
            // Commit the session (this will start the installation workflow).
            Log.d(TAG, "Committing status receiver.");
            InstallerSession.commit(statusReceiver);
        } catch (IOException e) {
            bHasFailed = true;
            PixoEventNotifier.getInstance().InstallationFailed(PackageName);
            Log.e(TAG, "Failed to add APK to install session. " + e.toString());
        } catch (RuntimeException e) {
            bHasFailed = true;
            PixoEventNotifier.getInstance().InstallationFailed(PackageName);
            if (InstallerSession != null) {
                InstallerSession.abandon();
            }
        Log.e(TAG, "Failed to install APK. " + e.toString());
        }

        if(bHasFailed == false) {
            PixoEventNotifier.getInstance().InstallationCompleted(PackageName);
        }
    }

    private void addApkToInstallSession(String assetName, PackageInstaller.Session session)
        throws IOException {
            // It's recommended to pass the file size to openWrite(). Otherwise installation may fail
            // if the disk is almost full.
            try (OutputStream packageInSession = session.openWrite("package", 0, -1);
            InputStream is = AppContext.getAssets().open(assetName)) {
            byte[] buffer = new byte[16384];
            int n;
            while ((n = is.read(buffer)) >= 0) {
                packageInSession.write(buffer, 0, n);
            }
        }
    }

    @Override
    public void Uninstall(String packageName) {
    }
}