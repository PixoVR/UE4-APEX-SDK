package com.Pixo.PixoPlatform;

public interface PixoInstallerInterface {
	public void Install(String packageName, PixoItemPackage apkPackage);
	public void Uninstall(String packageName);
}