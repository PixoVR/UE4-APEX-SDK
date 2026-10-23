package com.Pixo.PixoPlatform;

import android.app.admin.DeviceAdminReceiver;
import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import android.Manifest.permission;


public class DeviceOwnerReceiver extends DeviceAdminReceiver {
    private final static String TAG = "PPlatform";
    private static String ApplicationName = "";

    public static void setTargetApplicationName(String InApplicationName) {
        Log.e(TAG, "Device Admin Receiver - Setting application to " + InApplicationName);
        ApplicationName = InApplicationName;
    }

    @Override
    public void onEnabled(Context context, Intent intent) {
        Log.e(TAG, "Device Admin Receiver - onEnabled");
    }

    @Override
    public void onDisabled(Context context, Intent intent) {
        Log.e(TAG, "Device Admin Receiver - onDisabled");
    }

    @Override
    public void onProfileProvisioningComplete(Context context, Intent intent) {
        Log.e(TAG, "Device Admin Receiver - onProfileProvisioningComplete");
        DevicePolicyManager dpm = (DevicePolicyManager) context.getSystemService(Context.DEVICE_POLICY_SERVICE);
        ComponentName admin = new ComponentName(context, DeviceOwnerReceiver.class);
        dpm.setProfileName(admin, "PixoVR Hub");

        if(dpm.setPermissionGrantState(admin, ApplicationName, permission.INSTALL_PACKAGES, DevicePolicyManager.PERMISSION_GRANT_STATE_GRANTED))
        {
            Log.e(TAG, "Added install permissions.");
        }
        else
        {
            Log.e(TAG, "Failed to add install permissions.");
        }
    }
}
