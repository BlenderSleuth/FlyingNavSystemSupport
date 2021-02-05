// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class FlyingNavBenchmark : ModuleRules
{
	public FlyingNavBenchmark(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "NavigationSystem", "FlyingNavSystem" });
		
		// Uncomment this line from FlyingNavSystem.Build.cs for this project to compile
		PublicDefinitions.Add("PATH_BENCHMARK=1"); 
	}
}
