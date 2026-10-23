package com.Pixo.PixoPlatform;

import android.content.Context;
import android.content.Intent;
import android.os.AsyncTask;
import android.util.Log;
import android.os.Environment;
import java.io.InputStream;
import java.io.FileInputStream;
import java.io.OutputStream;
import java.io.FileOutputStream;
import java.io.File;
import java.io.IOException;

public class PixoAsyncMoveFile extends AsyncTask<PixoItemPackage, Integer, Integer> {
    protected static final String TAG = "PixoAsyncMoveFile";
    Context AppContext = null;
	PixoItemPackage TargetItemPackage = null;

    public native void nativePVRInstallationCompletedCallback(String packagename);
    public native void nativePVRInstallationFailedCallback(String packagename);

    public PixoAsyncMoveFile(Context InAppContext) {
        AppContext = InAppContext;
    }

    @Override
    protected Integer doInBackground(PixoItemPackage... InItemPackages) {
		this.TargetItemPackage = InItemPackages[0];
        try {
            if(TargetItemPackage.ItemFiles.size() >= 2) {
                Log.e(TAG, "More than 1 file, moving files.");
                int ItemIndex = 0;
                for(String ItemFile : TargetItemPackage.ItemFiles) {
                    if(ItemFile == null) {
                        ItemIndex += 1;
                        continue;
                    }

                    Log.e(TAG, "File is not null.");

                    if(ItemIndex < 1) {
                        Log.e(TAG, "First file is skipped.");
                        ItemIndex += 1;
                        continue;
                    }

                    try {
                        Log.e(TAG, "Moving item file " + ItemFile + " for package " + TargetItemPackage.PackageName);
                        moveTargetFile(TargetItemPackage.PackageName, ItemFile);
                    }
                    catch(Exception e) {
                        Log.e(TAG, "File error occurred, ignoring file " + ItemFile);
                        e.printStackTrace();
                    }
                    ItemIndex += 1;
                }
            }
            return 0;
        } catch (Exception e) {
            PixoEventNotifier.getInstance().InstallationFailed(TargetItemPackage.PackageName);
            e.printStackTrace();
        }
        return 1;
    }

    @Override
    protected void onPreExecute() {
        super.onPreExecute();
    }

    @Override
    protected void onPostExecute(Integer b) {
        super.onPostExecute(b);

        Log.i(TAG, "Move File completed.");
        PixoEventNotifier.getInstance().InstallationCompleted(TargetItemPackage.PackageName);
    }

    @Override
    protected void onProgressUpdate(Integer... progress)
    {
        // Will be fired when publishProgress() is called.
    }

    public boolean moveTargetFile(String InPackageName, String InTargetFile) throws Exception {

        try {
            File TargetFile = new File(InTargetFile);
            File DestinationFile = new File(Environment.getExternalStorageDirectory() + "/Android/obb/" + InPackageName + "/" + TargetFile.getName());

            Log.e(TAG, "Attempting to Move target file: " + InTargetFile + " to new destination: " + DestinationFile.getParent());

            File DestinationFileParent = new File(DestinationFile.getParent());
            boolean DestinationDirectoryCreated = true;
            if(!DestinationFileParent.exists()) {
                DestinationDirectoryCreated = DestinationFileParent.mkdirs();
            }

            if(!DestinationDirectoryCreated) {
                Log.e(TAG, "Failed to create directory.");
                return false;
            }

            Log.e(TAG, "Moving target file: " + InTargetFile + " to new destination: " + DestinationFile.getAbsolutePath());
            if(!TargetFile.renameTo(DestinationFile)) {
                if(!moveFile(TargetFile, DestinationFile)) {
                    Log.d(TAG, "Failed to move OBB file: " + InTargetFile + " to new destination: " + DestinationFile.getAbsolutePath());
                    return false;
                }
            }
        }
        catch(Exception e) {
            Log.e(TAG, e.toString());
            throw new Exception(e);
        }

        return true;
    }

    private boolean moveFile(File sourceFile, File destFile) throws IOException {
        InputStream in = new FileInputStream(sourceFile);
        OutputStream out = new FileOutputStream(destFile);

        // Copy the bits from instream to outstream
        byte[] buf = new byte[8192];
        int len;

        Log.d(TAG, "Start moving OBB using Streams");
        while ((len = in.read(buf)) > 0) {
            out.write(buf, 0, len);
        }

        in.close();
        out.close();

        return true;
    }
}
