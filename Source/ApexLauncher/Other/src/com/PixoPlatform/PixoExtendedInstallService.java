package com.Pixo.PixoPlatform;

import android.content.Intent;
import android.util.Log;

/**
 * The installer for headsets that cannot install packages (Quest, HTC Business, other Android
 * headsets): only Pico's ToB service and HTC Focus's library may install. UApexLauncherSubsystem
 * refuses installs on these headsets before downloading; this reports failure if one gets here anyway.
 */
public class PixoExtendedInstallService extends PixoInstallerService {
    public PixoExtendedInstallService() {
        super("PixoExtendedInstallService");
    }

    public PixoExtendedInstallService(String name) {
        super(name);
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
        Log.e(TAG, "This headset cannot install packages; " + packageName + " was not installed.");
        PixoEventNotifier.getInstance().InstallationFailed(packageName);
    }

    @Override
    public void Uninstall(String packageName) {
        Log.e(TAG, "This headset cannot uninstall packages; " + packageName + " was not uninstalled.");
        PixoEventNotifier.getInstance().UninstallationFailed(packageName);
    }
}
