package com.Pixo.PixoPlatform;

import android.os.Parcel;
import android.os.Parcelable;
import java.util.ArrayList;
import android.util.Log;

public class PixoItemPackage implements Parcelable {
    protected static final String TAG = "PPlatform";

    public String ApkFile;
    public long ApkSize;
    public String ObbFile;
    public ArrayList<String> ItemFiles;
    public String PackageName;

    public PixoItemPackage(String InApkFile, String InObbFile, String InPackageName) {
        Log.d(TAG, "Constructor");
        ApkFile = InApkFile;
        ApkSize = 0;
        ObbFile = InObbFile;
        ItemFiles = new ArrayList<>();
        ItemFiles.add(ApkFile);
        ItemFiles.add(ObbFile);
        PackageName = InPackageName;
    }

    protected PixoItemPackage(Parcel InParcel) {
        Log.d(TAG, "Parcel Constructor");
        ItemFiles = new ArrayList<>();
        ApkFile = InParcel.readString();
        ApkSize = InParcel.readLong();
        ObbFile = InParcel.readString();
        PackageName = InParcel.readString();
        ItemFiles = InParcel.readArrayList(null);
        Log.e(TAG, "In Apk File: " + ApkFile);
        Log.e(TAG, "In OBB File: " + ObbFile);
        Log.e(TAG, "In File count: " + ItemFiles.size());
        Log.e(TAG, "In Package name: " + PackageName);
    }

    @Override
    public void writeToParcel(Parcel OutParcel, int PacelFlags) {
        OutParcel.writeString(ApkFile);
        OutParcel.writeLong(ApkSize);
        OutParcel.writeString(ObbFile);
        OutParcel.writeString(PackageName);
        OutParcel.writeList(ItemFiles);
        Log.e(TAG, "Out Apk File: " + ApkFile);
        Log.e(TAG, "Out OBB File: " + ObbFile);
        Log.e(TAG, "Out File count: " + ItemFiles.size());
        Log.e(TAG, "Out Package name: " + PackageName);
    }

    @Override
    public int describeContents() {
        return 0;
    }

    long getSize() {
        return ApkSize;
    }

    public static final Creator<PixoItemPackage> CREATOR = new Creator<PixoItemPackage>() {
        @Override
        public PixoItemPackage createFromParcel(Parcel in) {
            return new PixoItemPackage(in);
        }

        @Override
        public PixoItemPackage[] newArray(int size) {
            return new PixoItemPackage[size];
        }
    };

    void addFile(String FileName) {
        ItemFiles.add(FileName);
    }

    void setAPK(String Apk) {
        ApkFile = Apk;
        ItemFiles.set(0, ApkFile);
    }

    void setOBB(String Obb) {
        ObbFile = Obb;
        ItemFiles.set(1, ObbFile);
    }

    String getAPK() {
        return ApkFile;
    }

    String getOBB() {
        return ObbFile;
    }
}
