package com.Pixo.PixoPlatform;

import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.Build;
import android.provider.Settings;
import android.util.Log;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.lang.reflect.Method;
import java.net.NetworkInterface;
import java.util.Collections;
import java.util.List;
import java.util.ArrayList;

public class PixoUtilities {
    private final static String TAG = "PixoUtilities";
    
    public static String getSerialNumber() 
    {
        String serialNumber;

        try {
            Class<?> c = Class.forName("android.os.SystemProperties");
            Method get = c.getMethod("get", String.class);

            serialNumber = (String) get.invoke(c, "gsm.sn1");
            if (serialNumber.equals(""))
                serialNumber = (String) get.invoke(c, "ril.serialnumber");
            if (serialNumber.equals(""))
                serialNumber = (String) get.invoke(c, "ro.serialno");
            if (serialNumber.equals(""))
                serialNumber = (String) get.invoke(c, "sys.serialnumber");
            if (serialNumber.equals(""))
                serialNumber = Build.SERIAL;

            // If none of the methods above worked
            if (serialNumber.equals(""))
                serialNumber = "";
        } catch (Exception e) {
            e.printStackTrace();
            serialNumber = "";
        }

        return serialNumber;
    }
    
    /** The wlan0 MAC address as hex without separators, or "" when it is unavailable. */
    public static String getMacAddress()
    {
        try {
            List<NetworkInterface> networkInterfaces = Collections.list(NetworkInterface.getNetworkInterfaces());
            for (NetworkInterface networkInterface : networkInterfaces) {
                if (networkInterface == null || !networkInterface.getName().equalsIgnoreCase("wlan0")) {
                    continue;
                }

                byte[] macBytes = networkInterface.getHardwareAddress();
                if (macBytes == null) {
                    return "";
                }

                StringBuilder macBuilder = new StringBuilder();
                for (byte macByte : macBytes) {
                    macBuilder.append(String.format("%02X", macByte));
                }

                return macBuilder.toString();
            }
        } catch (Exception exception) {
            Log.e(TAG, "Failed to read the MAC address: " + exception);
        }

        return "";
    }

    public static String getAndroidID(android.content.Context context) {
        return Settings.Secure.getString(context.getContentResolver(), Settings.Secure.ANDROID_ID);
    }

    public static boolean isPackageInstalled(final Context context, String packagename)
	{
        Log.e(TAG, "IsPackageInstalled - Checking if " + packagename + " is installed.");

        try
        {
            List<String> packages = getInstalledPackages(context);
            return packages.contains(packagename);
        }
        catch(Exception e)
        {
            Log.e(TAG, "Install check exception: " + e);
        }

        Log.e(TAG, "Looks like an exception happened.");
        return false;
    }

    public static void logAllPackages(final Context context)
    {
        Log.e(TAG, "Logging all packages.");

        try
        {
            List<String> packages = getInstalledPackages(context);
            for (String installedPackage : packages) {
                Log.e(TAG, "Package installed: " + installedPackage);
            }
        }
        catch(Exception e)
        {
            Log.e(TAG, "Package check exception: " + e);
        }
        
        Log.e(TAG, "Retrieved all package names and output them to the log.");
    }

    public static List<String> getInstalledPackages(final Context context)
    {
        List<String> result = new ArrayList<>();
        final PackageManager pm = context.getPackageManager();
        try
		{
            List<PackageInfo> apps = pm.getInstalledPackages(0);
            for (PackageInfo packageInfo : apps) {
                result.add(packageInfo.packageName);
            }
            return result;
        }
		catch (Exception ignored)
		{
            // could have failed depending on bugs in Android version
        }

        // use fallback
        BufferedReader bufferedReader = null;
        try
		{
            Process process = Runtime.getRuntime().exec("pm list packages");
            bufferedReader = new BufferedReader(new InputStreamReader(process.getInputStream()));
            String line;
            while ((line = bufferedReader.readLine()) != null) {
                final String packageName = line.substring(line.indexOf(':') + 1);
                result.add(packageName);
            }
            bufferedReader.close();
            process.waitFor();
        }
		catch (Exception e)
		{
            e.printStackTrace();
        }
        if (bufferedReader != null)
        {
            try
			{
                bufferedReader.close();
            }
            catch (Exception e) {
            }
        }
        return result;
    }
}
