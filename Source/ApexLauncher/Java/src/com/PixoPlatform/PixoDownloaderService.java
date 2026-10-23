package com.Pixo.PixoPlatform;

import android.app.DownloadManager;
import android.app.IntentService;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.os.Bundle;
import android.os.ResultReceiver;
import android.util.Log;

import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.StringBuilder;
import java.net.HttpURLConnection;
import javax.net.ssl.HttpsURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

public class PixoDownloaderService extends IntentService {
    protected static final String TAG = "PPlatform";

	public class DownloadProgress {
		public boolean DownloadFinished;
		public int Progress;

		public DownloadProgress() {
			DownloadFinished = false;
			Progress = 0;
		}
	}

	// Bundle parcelables
	public static final String DOWNLOAD_INFO = "download_info";
	public static final String DOWNLOAD_URL = "download_url";
	public static final String DOWNLOAD_NAME = "download_name";
	public static final String DOWNLOADER_RECEIVER = "download_receiver";
	public static final String DOWNLOAD_SIZE = "download_size";
	public static final String DOWNLOAD_CONTEXT = "download_context";

	// Status strings
	public static final String DOWNLOAD_STARTED = "download_started";
	public static final String DOWNLOAD_COMPLETED = "download_completed";
	public static final String DOWNLOAD_CANCELED = "download_canceled";
	public static final String DOWNLOAD_FAILED = "download_failed";
	public static final String DOWNLOAD_EXTRACTION_STARTED = "download_extracting_started";
	public static final String DOWNLOAD_EXTRACTION_COMPLTED = "download_extracting_completed";
	public static final String DOWNLOAD_EXTRACTION_PROGRESS = "download_extracting_progress";
	public static final String DOWNLOADER_READY_TO_INSTALL = "download_ready_to_install";
	public static final String DOWNLOAD_PROGRESS = "download_progress";
	public static final String DOWNLOAD_PACKAGE = "download_package";

	public static int STATUS_OK = 222;
	public static int STATUS_FAILED = 333;
	public static int PROGRESS_EVENT_SIZE = 1024 * 1024 * 10; // Number of bytes should be processed when informing the listener.
	
	public boolean CancelDownload = false;
	public long TotalDownloadSize = -1;
	private String DestinationDirectory = null;
	private String OutputFilePath = null;

	public PixoDownloaderService() {
		super("PixoDownloaderService");
	}

	public PixoDownloaderService(String name) {
		super(name);
	}

	@Override
	public void onDestroy() {
		super.onDestroy();
		CancelDownload = true;
	}

	@Override
	protected void onHandleIntent(Intent InIntent) {
		Log.e(TAG, "Handling PPlatform download service intent.");
		Bundle IntentBundle = InIntent.getExtras();
		if (null == IntentBundle) {
			Log.e(TAG, "Missing intent bundle.");		
			return;
		}

		if (!IntentBundle.containsKey(DOWNLOAD_URL)) {
			Log.e(TAG, "Intent bundle is missing download URL.");
			return;
		}

		ResultReceiver DownloadResultReceiver = IntentBundle.getParcelable(DOWNLOADER_RECEIVER);
		String DownloadUrl = IntentBundle.getString(DOWNLOAD_URL);
		String ApkName = IntentBundle.getString(DOWNLOAD_NAME);

		InputStream input = null;
		OutputStream output = null;
		File DownloadOutputFile = null;
		CancelDownload = false;

		Log.e(TAG, "Getting path to place file.");
		// Will have to point to external cache dir because the Download Manager cannot reach internal memory.
		DestinationDirectory = PixoDownloaderService.this.getExternalCacheDir().getAbsolutePath();

		long CurrentDownloadSize = 0;
		
		try {
			Log.e(TAG, "Attempting to download file " + DownloadUrl + " and install it to " + DestinationDirectory);

			DownloadOutputFile = getDownloadDestinationFile(DownloadResultReceiver, DownloadUrl, ApkName);
			Log.e(TAG, "Absolute file path: " + DownloadOutputFile.getAbsolutePath());

			String DownloadFileName = getFileName(DownloadUrl);

			Uri OutputUri = Uri.fromFile(DownloadOutputFile);
			Log.e(TAG, "Building a request to path + " + OutputUri + "!");
			Log.e(TAG, "External path: " + PixoDownloaderService.this.getExternalCacheDir());
			DownloadManager.Request DownloadRequest = new DownloadManager.Request(Uri.parse(DownloadUrl))
				.setNotificationVisibility(DownloadManager.Request.VISIBILITY_HIDDEN)// Visibility of the download Notification
				.setDestinationUri(OutputUri)// Uri of the destination file
				.setTitle(DownloadFileName)// Title of the Download Notification
				.setDescription("Downloading")// Description of the Download Notification
				.setRequiresCharging(false)// Set if charging is required to begin the download
				.setAllowedOverMetered(true)// Set if download is allowed on Mobile network
				.setAllowedOverRoaming(true);// Set if download is allowed on roaming network

			Log.e(TAG, "Get download manager.");
			DownloadManager DownloadManagerService = (DownloadManager)getSystemService(DOWNLOAD_SERVICE);
			long DownloadID = -1;
			if(DownloadManagerService != null)
			{
				Log.e(TAG, "Enqueue request in download manager.");
				DownloadID = DownloadManagerService.enqueue(DownloadRequest);
			}
			else
			{
				Log.e(TAG, "Failed to get download manager.");
				return;
			}
			
			Log.e(TAG, "Request enqueued for ID " + DownloadID + ".");
			DownloadProgress DownloadProgressTracker = new DownloadProgress();
			downloadStarted(DownloadResultReceiver, ApkName, 0);
			StringBuilder OutputFilePathBuilder = new StringBuilder("");
			while (!DownloadProgressTracker.DownloadFinished) {
				DownloadProgressTracker = processDownload(DownloadManagerService, DownloadResultReceiver, DownloadID, OutputFilePathBuilder, ApkName, DownloadProgressTracker);
			}

			Log.e(TAG, "Download successful to file path " + OutputFilePathBuilder.toString());

			sendProgress(100, DownloadResultReceiver, ApkName);
			downloadCompleted(DownloadResultReceiver, ApkName);
			
			Log.e(TAG, "Getting ready to start extracting.");

			PixoAsyncExtractor ExtractorTask = new PixoAsyncExtractor(this, DownloadManagerService, this, DownloadResultReceiver, DestinationDirectory, OutputFilePath);

			Log.e(TAG, "Extractor task created.");

            ExtractorTask.execute(ApkName);

			Log.e(TAG, "Extractor task executed.");

		} catch (MalformedURLException e) {
			Log.e(TAG, "Bad URL passed.");
			e.printStackTrace();
		} catch (IOException e) {
			Log.e(TAG, "Could not make connection to download.");
			e.printStackTrace();
		} catch (Exception e) {
			Log.e(TAG, "Caught a generic exception.");
			Log.e(TAG, e.getMessage());
			e.printStackTrace();
		}
	}

	protected File getDownloadDestinationFile(ResultReceiver DownloadResultReceiver, String DownloadUrl, String ApkName) throws Exception
	{
			Log.e(TAG, "Attempting to download file " + DownloadUrl + " and install it to " + DestinationDirectory);
			String FileExtension = getFileExtension(DownloadUrl);
			FileExtension = FileExtension.substring(0, 3);
			if(!FileExtension.contains("zip"))
			{
				Log.e(TAG, "Trying to download a non-zip file. Stopping download.");
				downloadCanceled(DownloadResultReceiver, ApkName);
				throw new Exception("Download file type is invalid.");
			}
			String FileName = getFileName(DownloadUrl);

			return File.createTempFile("tmp", "." + FileExtension, new File(DestinationDirectory));
	}

	protected DownloadProgress processDownload(DownloadManager Downloader, ResultReceiver DownloadResultReceiver, long DownloadId, StringBuilder OutputPathBuilder, String ApkName, DownloadProgress CurrentDownloadProgress)
	{
		Log.e(TAG, "Processing download.");
		Cursor DownloadCursor = Downloader.query(new DownloadManager.Query().setFilterById(DownloadId));
		if (DownloadCursor.moveToFirst()) {
			int DownloadStatus = DownloadCursor.getInt(DownloadCursor.getColumnIndex(DownloadManager.COLUMN_STATUS));
			switch (DownloadStatus) {
				case DownloadManager.STATUS_FAILED: {
					int DownloadFailedReason = DownloadCursor.getInt(DownloadCursor.getColumnIndex(DownloadManager.COLUMN_REASON));
					Log.e(TAG, "Failed to download with reason code " + DownloadFailedReason);
					downloadCanceled(DownloadResultReceiver, ApkName);
					CurrentDownloadProgress.DownloadFinished = true;
					break;
				}
				case DownloadManager.STATUS_PAUSED:
					Log.e(TAG, "Download pause!");
					break;
				case DownloadManager.STATUS_PENDING: {
					Log.e(TAG, "Download pending!");
					sendProgress(0, DownloadResultReceiver, ApkName);
					CurrentDownloadProgress.DownloadFinished = false;
					CurrentDownloadProgress.Progress = 0;
					break;
				}
				case DownloadManager.STATUS_RUNNING: {
					Log.e(TAG, "Download is running!");
					boolean DownloadFirstPass = false;
					if(TotalDownloadSize < 0)
					{
						TotalDownloadSize = DownloadCursor.getLong(DownloadCursor.getColumnIndex(DownloadManager.COLUMN_TOTAL_SIZE_BYTES));
						DownloadFirstPass = true;
					}

					if (TotalDownloadSize >= 0)
					{
						if(DownloadFirstPass == true)
						{
							Log.e(TAG, "Download total size is " + TotalDownloadSize + "!");
							downloadStarted(DownloadResultReceiver, ApkName, (int)TotalDownloadSize);
						}
						final long DownloadedAmount = DownloadCursor.getLong(DownloadCursor.getColumnIndex(DownloadManager.COLUMN_BYTES_DOWNLOADED_SO_FAR));
						int DownloadProgress = (int) ((DownloadedAmount * 100L) / TotalDownloadSize);
						Log.e(TAG, "Download progress is " + DownloadProgress + "!");
						if(DownloadProgress - CurrentDownloadProgress.Progress >= 10)
						{
							TotalDownloadSize = DownloadCursor.getLong(DownloadCursor.getColumnIndex(DownloadManager.COLUMN_TOTAL_SIZE_BYTES));
							Log.e(TAG, "Download amount is " + DownloadedAmount + " out of total " + TotalDownloadSize);
							sendProgress(DownloadProgress, DownloadResultReceiver, ApkName);
							CurrentDownloadProgress.Progress = DownloadProgress;
						}
					}
					break;
				}
				case DownloadManager.STATUS_SUCCESSFUL: {
					OutputFilePath = DownloadCursor.getString(DownloadCursor.getColumnIndex(DownloadManager.COLUMN_LOCAL_URI));
					OutputFilePath = OutputFilePath.replace("file://","");
					Log.e(TAG, "Download of file " + OutputFilePath + " was successful!");
					OutputPathBuilder.append(OutputFilePath);
					int DownloadProgress = 100;
					sendProgress(DownloadProgress, DownloadResultReceiver, ApkName);
					CurrentDownloadProgress.Progress = DownloadProgress;
					CurrentDownloadProgress.DownloadFinished = true;

					break;
				}
			}
		}
		DownloadCursor.close();
		return CurrentDownloadProgress;
	}

	public void downloadStarted(ResultReceiver DownloadResultReceiver, String DownloadName, int DownloadSize) {
		Bundle bundle = new Bundle();
		bundle.putBoolean(PixoDownloaderService.DOWNLOAD_STARTED, true);
		bundle.putInt(PixoDownloaderService.DOWNLOAD_SIZE, DownloadSize);
		bundle.putString(PixoDownloaderService.DOWNLOAD_NAME, DownloadName);
		DownloadResultReceiver.send(STATUS_OK, bundle);
	}

	public void downloadCompleted(ResultReceiver DownloadResultReceiver, String DownloadName) {
		Bundle bundle = new Bundle();
		bundle.putBoolean(PixoDownloaderService.DOWNLOAD_COMPLETED, true);
		bundle.putString(PixoDownloaderService.DOWNLOAD_NAME, DownloadName);
		DownloadResultReceiver.send(STATUS_OK, bundle);
	}

	public void downloadCanceled(ResultReceiver DownloadResultReceiver, String DownloadName) {
		Bundle bundle = new Bundle();
		bundle.putBoolean(PixoDownloaderService.DOWNLOAD_CANCELED, true);
		bundle.putString(PixoDownloaderService.DOWNLOAD_NAME, DownloadName);
		DownloadResultReceiver.send(STATUS_FAILED, bundle);
	}

	public void downloadFailed(ResultReceiver DownloadResultReceiver, String DownloadName) {
		Bundle bundle = new Bundle();
		bundle.putBoolean(PixoDownloaderService.DOWNLOAD_FAILED, true);
		bundle.putString(PixoDownloaderService.DOWNLOAD_NAME, DownloadName);
		DownloadResultReceiver.send(STATUS_FAILED, bundle);
	}

	public void sendProgress(int Progress, ResultReceiver DownloadResultReceiver, String DownloadName) {
		Bundle bundle = new Bundle();
		bundle.putInt(PixoDownloaderService.DOWNLOAD_PROGRESS, Progress);
		bundle.putString(PixoDownloaderService.DOWNLOAD_NAME, DownloadName);
		DownloadResultReceiver.send(STATUS_OK, bundle);
	}

	public void sendExtractionProgress(int Progress, ResultReceiver DownloadResultReceiver, String DownloadName) {
		Bundle bundle = new Bundle();
		bundle.putInt(PixoDownloaderService.DOWNLOAD_EXTRACTION_PROGRESS, Progress);
		bundle.putString(PixoDownloaderService.DOWNLOAD_NAME, DownloadName);
		DownloadResultReceiver.send(STATUS_OK, bundle);
	}

	public void extractionStarted(ResultReceiver DownloadResultReceiver, String DownloadName) {
		Bundle bundle = new Bundle();
		bundle.putBoolean(PixoDownloaderService.DOWNLOAD_EXTRACTION_STARTED, true);
		bundle.putString(PixoDownloaderService.DOWNLOAD_NAME, DownloadName);
		DownloadResultReceiver.send(STATUS_OK, bundle);
	}

	public void extractionCompleted(ResultReceiver DownloadResultReceiver, String DownloadName) {
		Bundle bundle = new Bundle();
		bundle.putBoolean(PixoDownloaderService.DOWNLOAD_EXTRACTION_COMPLTED, true);
		bundle.putString(PixoDownloaderService.DOWNLOAD_NAME, DownloadName);
		DownloadResultReceiver.send(STATUS_OK, bundle);
	}

	public void readyToInstall(ResultReceiver DownloadResultReceiver, String DownloadName, PixoItemPackage Item) {
		Bundle bundle = new Bundle();
		bundle.putBoolean(PixoDownloaderService.DOWNLOADER_READY_TO_INSTALL, true);
		bundle.putParcelable(DOWNLOAD_PACKAGE, Item);
		bundle.putString(PixoDownloaderService.DOWNLOAD_NAME, DownloadName);
		DownloadResultReceiver.send(STATUS_OK, bundle);
	}

	public PixoItemPackage unzipPackage(ResultReceiver DownloadResultReceiver, String DestinationDirectory, String ZipFilePath, String ApkName) throws IOException {
		Log.e(TAG, "Unzipping package.");
		PixoItemPackage ItemPackage = new PixoItemPackage(null, null, ApkName);
		String ZipFile = ZipFilePath;
		DestinationDirectory = Files.createTempDirectory(Paths.get(DestinationDirectory), "tmpdir").toString();

		Log.e(TAG, "Zip File: " + ZipFile);
		Log.e(TAG, "Destination Dir: " + DestinationDirectory);

		File ZipArchive = new File(ZipFile);
		try {
			FileInputStream FileStream = new FileInputStream(ZipArchive);
			ZipInputStream ZipStream = new ZipInputStream(new BufferedInputStream(FileStream));

			ZipEntry ZipItemEntry;
			while ((ZipItemEntry = ZipStream.getNextEntry()) != null) {
				Log.e(TAG, "Unzipping entry " + ZipItemEntry.getName());
				ItemPackage = unzipEntry(DownloadResultReceiver, ZipStream, ZipItemEntry, DestinationDirectory, ItemPackage);
			}
		} catch (Exception e) {
			Log.e(TAG, "Unzip exception", e);
		}

		return ItemPackage;
	}

	private PixoItemPackage unzipEntry(ResultReceiver DownloadResultReceiver, ZipInputStream ZipStream, 
										ZipEntry ItemEntry, String DestinationDirectory, PixoItemPackage Item) throws IOException {
		Log.e(TAG, "Starting to unzip entry.");
		if (ItemEntry.isDirectory()) {
			Log.e(TAG, "Entry is a directory: " + ItemEntry.getName());
			createDirectory(new File(DestinationDirectory, ItemEntry.getName()));
			return Item;
		}

		Log.e(TAG, "Creating output directory: " + DestinationDirectory + "/" + ItemEntry.getName());
		File OutputFile = new File(DestinationDirectory, ItemEntry.getName());

		if (!OutputFile.getParentFile().exists()) {
			createDirectory(OutputFile.getParentFile());
		}

		Log.e(TAG, "Extracting: " + ItemEntry);

		BufferedOutputStream OutputStream = new BufferedOutputStream(Files.newOutputStream(Paths.get(OutputFile.getPath())));
		long UncompressedSize = ItemEntry.getSize();
		long CurrentReadAmount = 0;

		boolean ShouldWrite = false;
		String FileExtension = getFileExtension(ItemEntry.getName());
		if (FileExtension.equals("apk") || FileExtension.equals("obb")) {
			ShouldWrite = true;
		}

		int PreviousProgress = 0;
		if(ShouldWrite == true) {
			int ReadBytes;
			byte [] Buffer = new byte[8192];
			try {
				try {
					while( (ReadBytes = ZipStream.read(Buffer)) != -1) {
						OutputStream.write(Buffer, 0, ReadBytes);
						CurrentReadAmount += ReadBytes;

						if ((null != DownloadResultReceiver) && (0 != (CurrentReadAmount % PROGRESS_EVENT_SIZE))) {
							int Progress = (int) ((CurrentReadAmount * 100) / UncompressedSize);
							if(Progress >= PreviousProgress+10)
							{
								sendExtractionProgress(Progress, DownloadResultReceiver, Item.PackageName);
								PreviousProgress = Progress;
							}
						}

					}
				} finally {
					OutputStream.close();
				}
			} finally {
				OutputStream.close();
			}
		}
		else {
			return Item;
		}

		if (FileExtension.equals("apk"))
		{
			Item.setAPK(OutputFile.getAbsolutePath());
			Item.ApkSize = UncompressedSize;
			Item.PackageName = getPackageName(OutputFile);
			Log.e(TAG, "Found APK inside zip: " + ItemEntry.getName() + "Package: " + Item.PackageName);
		} 
		else if (FileExtension.equals("obb"))
		{
			Item.setOBB(OutputFile.getAbsolutePath());
			Log.e(TAG, "Found OBB inside zip: " + ItemEntry.getName());
		}
		else
		{
			Item.addFile(OutputFile.getAbsolutePath());
			Log.e(TAG, "Found a normal file.");
		}
		return Item;
	}

	private void createDirectory(File dir) {
		if (dir.exists()) {
			return;
		}

		if (!dir.mkdirs()) {
			throw new RuntimeException("Can not create dir " + dir);
		}
	}

	private boolean isInternetConnectionAvailable() {
		ConnectivityManager cm = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
		NetworkInfo netInfo = cm.getActiveNetworkInfo();

		if (netInfo != null) {
			if (netInfo.isConnectedOrConnecting() && netInfo.isConnected() && netInfo.isAvailable()) {
				return true;
			}
		}

		return false;
	}

	public String getPackageName(File file) {
		PackageManager pm = getPackageManager();
		PackageInfo packageInfo = pm.getPackageArchiveInfo(file.getAbsolutePath(), 0);

		return packageInfo.packageName;
	}

	public String getFileExtension(String FileName) {
		return FileName.substring(FileName.lastIndexOf(".") + 1);
	}

	public String getFileName(String FileName) {
		return FileName.substring(FileName.lastIndexOf("/") + 1);
	}
}
