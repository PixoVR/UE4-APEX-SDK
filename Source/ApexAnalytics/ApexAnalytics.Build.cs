// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

using UnrealBuildTool;

// Components and systems that record what the player does (tracked objects, interactables, gaze,
// steps, poses) and pass it to analytics providers, the built-in one sending it to Apex.
public class ApexAnalytics : ModuleRules
{
	public ApexAnalytics(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "ApexSDK" });
		PrivateDependencyModuleNames.AddRange(new string[] { "ApexCore", "EyeTracker" });
	}
}
