package com.Pixo.PixoPlatform;

import android.content.BroadcastReceiver;
import android.content.Intent;
import android.content.Context;
import android.util.Log;

public class PackageInstallerStatusReceiver extends BroadcastReceiver
{
    private static final String TAG = "PackageInstallerStatusReceiver";

    @Override
    public void onReceive(Context context, Intent intent)
    {
        Log.e(TAG, "Intent action: " + intent.getAction());
        Log.e(TAG, "updated");
    }
}