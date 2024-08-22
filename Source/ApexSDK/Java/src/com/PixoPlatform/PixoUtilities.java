package com.Pixo.PixoPlatform;

import android.os.Build;
import java.lang.reflect.Method;
import android.util.Log;
import android.Manifest.permission;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiInfo;
import java.net.NetworkInterface;
import java.util.List;
import java.util.Collections;

public class PixoUtilities {
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

    public static String getMacAddress()
    {
        Log.d("PixoUtilities", "Getting mac address.");
        try {
            List<NetworkInterface> ifaceList = Collections.list(NetworkInterface.getNetworkInterfaces());
            for (NetworkInterface iface : ifaceList) {
                if (iface == null) {
                    continue;
                }
                if (!iface.getName().equalsIgnoreCase("wlan0")) {
                    continue;
                }
                byte[] macBytes = iface.getHardwareAddress();
                if (macBytes == null) {
                    return "";
                }
                StringBuilder macSb = new StringBuilder();
                for (byte bytes : macBytes) {
                    macSb.append(String.format("%02X", bytes));
                }
                if (macSb.length() == 0) {
                    return "";
                }
                return macSb.toString();
            }
        } catch (Exception ex) {
            ex.printStackTrace();
        }
        return "";
    }
}