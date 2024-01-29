// Copyright 2022 PixoVR Corp. All Rights Reserved.

using System.IO;

namespace UnrealBuildTool.Rules
{
	public class ApexSDK : ModuleRules
	{
		public ApexSDK(ReadOnlyTargetRules Target) : base(Target)
		{
			PrivateDefinitions.Add("APEXSDK_PACKAGE=1");
			PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
			PrecompileForTargets = PrecompileTargetsType.Any;

			PrivateIncludePaths.AddRange(
				new string[] {
					"ApexSDK/Private",
					// ... add other private include paths required here ...
				});

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Engine",
					"VaRest",
					"Json",
					"EngineSettings",
				});

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"WebSockets",
					"Projects"
				});

			if (Target.bBuildEditor == true)
			{
				PrivateDependencyModuleNames.Add("UnrealEd");
			}

			if (Target.Version.MajorVersion >= 5 && Target.Version.MinorVersion >= 2)
			{
				PublicDependencyModuleNames.AddRange(
					new string[]
					{
						"XRBase",
					}
				);
			}
			else
			{
				PublicDependencyModuleNames.AddRange(
					new string[]
					{
						"HeadMountedDisplay",
					}
				);
			}

			if (Target.bBuildEditor == true)
			{
				PublicDependencyModuleNames.Add("AndroidRuntimeSettings");
			}

			if (Target.Platform == UnrealTargetPlatform.Android)
			{
				PublicDependencyModuleNames.Add("Launch");
				string PluginPath = Utils.MakePathRelativeTo(ModuleDirectory, Target.RelativeEnginePath);
				AdditionalPropertiesForReceipt.Add(
					"AndroidPlugin",
					Path.Combine(PluginPath, "ApexSDKAndroid_UPL.xml")
				);
			}
		}
	}
}
