// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class HierarchyTableBuiltinRuntime : ModuleRules
{
	public HierarchyTableBuiltinRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		ShortName = "HTAnimRun";

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"HierarchyTableRuntime"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
            {
			}
		);
	}
}
