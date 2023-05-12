// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class ReadingTracker : ModuleRules
{
	public ReadingTracker(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bEnableExceptions = true;
		Definitions.Add("_USE_SCIVI_CONNECTION_");
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "VRExperimentsBase", "BaseReadingTracker" });

		PrivateDependencyModuleNames.AddRange(new string[] {"SRanipal", "SRanipalEye", "HeadMountedDisplay", "Json", "JsonUtilities", });
		
		PrivateIncludePaths.AddRange(new string[] {
			"../Plugins/VRExperimentsBase/Source/ThirdPartyLibs",
			"../Plugins/SRanipal/Source/SRanipal/Public/Eye"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
