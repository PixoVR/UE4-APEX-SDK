// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

// Compiles apex-sdk-core (Source/ThirdParty/ApexCore, copied in by Tools/update_core.py) into the
// game. Each file in Private/ includes one core source file, so the core stays byte-identical to
// its release, and only the HTTP backend for the target platform is compiled.
public class ApexCore : ModuleRules
{
	public ApexCore(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.CPlusPlus;
		PCHUsage = PCHUsageMode.NoPCHs;
		CppStandard = CppStandardVersion.Cpp17;

		// The core reports errors through exceptions internally and catches them at the C boundary.
		bEnableExceptions = true;

		// Core source files share names of file-local helpers; unity builds would merge them.
		bUseUnity = false;

		// Third-party code: Unreal's stricter warnings do not apply to it.
		bEnableUndefinedIdentifierWarnings = false;
		ShadowVariableWarningLevel = WarningLevel.Off;

		string coreDirectory = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", "ThirdParty", "ApexCore"));
		PublicIncludePaths.Add(Path.Combine(coreDirectory, "include"));
		PrivateIncludePaths.Add(coreDirectory);
		PrivateIncludePaths.Add(Path.Combine(coreDirectory, "src"));
		PrivateIncludePaths.Add(Path.Combine(coreDirectory, "third_party"));

		PrivateDependencyModuleNames.Add("Core");

		// In modular builds (the editor) ApexCore is its own DLL: export the C interface from it and
		// import it everywhere else. Monolithic games link it statically, where APEX_API stays empty.
		if (Target.LinkType == TargetLinkType.Modular)
		{
			PrivateDefinitions.Add("APEX_BUILD_SHARED");
			PublicDefinitions.Add("APEX_USE_SHARED");
		}

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PrivateDefinitions.Add("APEX_TRANSPORT_WINHTTP");
			PublicSystemLibraries.Add("winhttp.lib");
		}
		else if (Target.Platform == UnrealTargetPlatform.Android)
		{
			// Linked statically into the game: ApexSDK hands the core the JavaVM at startup.
			PrivateDefinitions.Add("APEX_TRANSPORT_ANDROID");
		}
		else
		{
			// No built-in backend: ApexSDK sends the core's requests through Unreal's HTTP module.
			PrivateDefinitions.Add("APEX_TRANSPORT_NONE");
		}
	}
}
