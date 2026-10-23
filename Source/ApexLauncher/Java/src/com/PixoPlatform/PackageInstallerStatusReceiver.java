package com.Pixo.PixoPlatform;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInstaller;
import android.util.Log;

/**
 * Receives PackageInstaller's outcome for an install or uninstall the standard installer started, and
 * reports it to PixoEventNotifier. When Android needs the user to confirm, it shows the confirmation.
 */
public class PackageInstallerStatusReceiver extends BroadcastReceiver
{
    private static final String TAG = "PackageInstallerStatusReceiver";
    public static final String kStatusAction = "PACKAGE_INSTALLED_ACTION";
    public static final String kExtraPackageName = "com.Pixo.PixoPlatform.PACKAGE_NAME";
    public static final String kExtraOperation = "com.Pixo.PixoPlatform.OPERATION";
    public static final String kOperationInstall = "install";
    public static final String kOperationUninstall = "uninstall";

    @Override
    public void onReceive(Context context, Intent intent)
    {
        String packageName = intent.getStringExtra(kExtraPackageName);
        boolean isUninstall = kOperationUninstall.equals(intent.getStringExtra(kExtraOperation));
        int status = intent.getIntExtra(PackageInstaller.EXTRA_STATUS, PackageInstaller.STATUS_FAILURE);
        Log.i(TAG, (isUninstall ? "Uninstall" : "Install") + " of " + packageName + ": status " + status);

        if (status == PackageInstaller.STATUS_PENDING_USER_ACTION)
        {
            Intent confirmationIntent = intent.getParcelableExtra(Intent.EXTRA_INTENT);
            if (confirmationIntent != null)
            {
                confirmationIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                context.startActivity(confirmationIntent);
            }

            return;
        }

        if (packageName == null)
        {
            return;
        }

        PixoEventNotifier eventNotifier = PixoEventNotifier.getInstance();
        if (status == PackageInstaller.STATUS_SUCCESS)
        {
            if (isUninstall)
            {
                eventNotifier.UninstallationCompleted(packageName);
            }
            else
            {
                eventNotifier.InstallationCompleted(packageName);
            }
        }
        else if (status == PackageInstaller.STATUS_FAILURE_ABORTED)
        {
            if (isUninstall)
            {
                eventNotifier.UninstallationCanceled(packageName);
            }
            else
            {
                eventNotifier.InstallationCanceled(packageName);
            }
        }
        else
        {
            Log.e(TAG, "Failed: " + intent.getStringExtra(PackageInstaller.EXTRA_STATUS_MESSAGE));
            if (isUninstall)
            {
                eventNotifier.UninstallationFailed(packageName);
            }
            else
            {
                eventNotifier.InstallationFailed(packageName);
            }
        }
    }
}
