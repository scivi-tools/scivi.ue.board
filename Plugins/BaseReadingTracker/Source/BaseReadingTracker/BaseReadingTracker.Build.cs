// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class BaseReadingTracker : ModuleRules
{
	public BaseReadingTracker(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		bEnableExceptions = true;
		PublicDefinitions.Add("_USE_SCIVI_CONNECTION_");
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "VRExperimentsBase", "UMG" });

		PrivateDependencyModuleNames.AddRange(new string[] { "SRanipal", "SRanipalEye", "HeadMountedDisplay", "Json", "JsonUtilities" });

		PrivateIncludePaths.AddRange(new string[] {
			"../../VRExperimentsBase/Source/ThirdPartyLibs",
			"../../SRanipal/Source/SRanipal/Public/Eye"
		});


		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
