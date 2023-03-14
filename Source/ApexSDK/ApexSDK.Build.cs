// Copyright 2022 PixoVR Corp.

using System.IO;

namespace UnrealBuildTool.Rules
{
	public class ApexSDK : ModuleRules
	{
		public ApexSDK(ReadOnlyTargetRules Target) : base(Target)
		{
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
					"HeadMountedDisplay"
				});
		}
	}
}
