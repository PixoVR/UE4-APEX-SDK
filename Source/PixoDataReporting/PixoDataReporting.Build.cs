// Copyright 2022 PixoVR Corp. All Rights Reserved.

using System.IO;

namespace UnrealBuildTool.Rules
{
	public class PixoDataReporting : ModuleRules
	{
		public PixoDataReporting(ReadOnlyTargetRules Target) : base(Target)
		{
			PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
			PrecompileForTargets = PrecompileTargetsType.Any;

			PrivateIncludePaths.AddRange(
				new string[] {
					"PixoDataReporting/Private",
					// ... add other private include paths required here ...
				});

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Engine",
					"EngineSettings",
					"VaRest",
					"Json",
					"HTTP",
				});

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"WebSockets",
					"Projects",
					"PixoConfig",
					"PixoAuth"
				});

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
		}
	}
}
