// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

// Launching, installing, updating and uninstalling apps; launch intents; shared files; the device
// serial number. The Android side is Java (Java/, Other/, Pico/, WaveVR/) added by the UPL file.
public class ApexLauncher : ModuleRules
{
	public ApexLauncher(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "DeveloperSettings" });

		if (Target.Platform == UnrealTargetPlatform.Android)
		{
			PrivateDependencyModuleNames.Add("Launch");
			string relativeModuleDirectory = Utils.MakePathRelativeTo(ModuleDirectory, Target.RelativeEnginePath);
			AdditionalPropertiesForReceipt.Add("AndroidPlugin",
				Path.Combine(relativeModuleDirectory, "ApexLauncherAndroid_UPL.xml"));
		}
	}
}
