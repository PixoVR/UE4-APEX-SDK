// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

using UnrealBuildTool;

// The Apex platform for Unreal: UApexSubsystem wraps apex-sdk-core (the ApexCore module) for login,
// module access and lists, sessions, telemetry and the launcher hand-off.
public class ApexSDK : ModuleRules
{
	public ApexSDK(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"DeveloperSettings",
			"ApexCore",
			"ApexLauncher",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Json",
			"HTTP",
			"WebSockets",
			"Sockets",
			"Projects",
			"HeadMountedDisplay",
		});

		if (Target.bBuildEditor)
		{
			// Sync Module Version writes the project and Android versions.
			PrivateDependencyModuleNames.Add("EngineSettings");
			PrivateDependencyModuleNames.Add("AndroidRuntimeSettings");
		}

		if (Target.Platform == UnrealTargetPlatform.Android)
		{
			PrivateDependencyModuleNames.Add("Launch");
		}
	}
}
