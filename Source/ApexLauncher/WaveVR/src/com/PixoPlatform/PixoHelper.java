package com.Pixo.PixoPlatform;

import android.app.Activity;
import android.content.Context;
import android.util.Log;
import java.lang.Class;

public class PixoHelper {
    public static Class GetInstallerClass() {
        return PixoExtendedInstallService.class;
    }
}