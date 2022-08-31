// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class ReadingTrackerTarget : TargetRules
{
	public ReadingTrackerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		//BuildEnvironment = TargetBuildEnvironment.Unique;// to resolve editing bForceEnableExceptions
		//bForceEnableExceptions = true;// enable exceptions for packaging
		DefaultBuildSettings = BuildSettingsVersion.V2;
		ExtraModuleNames.AddRange( new string[] { "ReadingTracker" } );
	}
}
