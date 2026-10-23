package com.Pixo.PixoPlatform;

import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.IntentSender;
import android.content.pm.PackageInstaller;
import android.util.Log;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * Installs and uninstalls through Android's PackageInstaller (Quest, HTC Business and other headsets).
 * The outcome arrives later in PackageInstallerStatusReceiver, which reports it to PixoEventNotifier.
 * Without device-owner rights, Android asks the user to confirm.
 */
public class PixoExtendedInstallService extends PixoInstallerService {
    // PendingIntent.FLAG_MUTABLE (API 31), spelled out so older compile SDKs build; PackageInstaller
    // must be able to add its status extras to the intent.
    private static final int kPendingIntentFlagMutable = 0x02000000;

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
        super.onHandleIntent(InIntent);

        switch (InstallActionType) {
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
        PackageName = packageName;
        Context context = AppContext != null ? AppContext : getApplicationContext();
        PixoEventNotifier.getInstance().InstallationStarted(packageName);

        PackageInstaller.Session installerSession = null;
        try {
            File apkFile = new File(apkPackage.ApkFile);
            PackageInstaller packageInstaller = context.getPackageManager().getPackageInstaller();
            PackageInstaller.SessionParams sessionParameters =
                new PackageInstaller.SessionParams(PackageInstaller.SessionParams.MODE_FULL_INSTALL);
            sessionParameters.setAppPackageName(packageName);
            int sessionId = packageInstaller.createSession(sessionParameters);
            installerSession = packageInstaller.openSession(sessionId);

            // Passing the size lets Android fail early when the disk is too full.
            try (OutputStream sessionStream = installerSession.openWrite("package", 0, apkFile.length());
                InputStream apkStream = new FileInputStream(apkFile)) {
                byte[] copyBuffer = new byte[65536];
                int readLength;
                while ((readLength = apkStream.read(copyBuffer)) >= 0) {
                    sessionStream.write(copyBuffer, 0, readLength);
                }

                installerSession.fsync(sessionStream);
            }

            installerSession.commit(MakeStatusReceiver(context, packageName, PackageInstallerStatusReceiver.kOperationInstall, sessionId));
            installerSession.close();
        } catch (IOException | RuntimeException exception) {
            Log.e(TAG, "Failed to install " + packageName + ": " + exception);
            if (installerSession != null) {
                installerSession.abandon();
            }

            PixoEventNotifier.getInstance().InstallationFailed(packageName);
        }
    }

    @Override
    public void Uninstall(String packageName) {
        PackageName = packageName;
        Context context = AppContext != null ? AppContext : getApplicationContext();
        PixoEventNotifier.getInstance().UninstallationStarted(packageName);
        try {
            context.getPackageManager().getPackageInstaller().uninstall(packageName,
                MakeStatusReceiver(context, packageName, PackageInstallerStatusReceiver.kOperationUninstall, 0));
        } catch (RuntimeException exception) {
            Log.e(TAG, "Failed to uninstall " + packageName + ": " + exception);
            PixoEventNotifier.getInstance().UninstallationFailed(packageName);
        }
    }

    private static IntentSender MakeStatusReceiver(Context context, String packageName, String operation, int requestCode) {
        Intent statusIntent = new Intent(context, PackageInstallerStatusReceiver.class);
        statusIntent.setAction(PackageInstallerStatusReceiver.kStatusAction);
        statusIntent.putExtra(PackageInstallerStatusReceiver.kExtraPackageName, packageName);
        statusIntent.putExtra(PackageInstallerStatusReceiver.kExtraOperation, operation);
        PendingIntent pendingIntent = PendingIntent.getBroadcast(context, requestCode, statusIntent,
            PendingIntent.FLAG_UPDATE_CURRENT | kPendingIntentFlagMutable);
        return pendingIntent.getIntentSender();
    }
}
