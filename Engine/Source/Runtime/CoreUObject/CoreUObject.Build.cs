// Copyright Epic Games, Inc. All Rights Reserved.

using System.Linq;
using UnrealBuildTool;

public class CoreUObject : ModuleRules
{
	public static void AddVerseVMDependencies(ModuleRules Rules, ReadOnlyTargetRules Target)
	{
		// If using the new VM either by default or directly, then add in a dependency to 
		// the core VerseVM which contains special compile flags not compatible with CoreUObject
		if (Target.bUseVerseVM)
		{
			Rules.PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"libpas",
				}
			);
		}
	}

	public CoreUObject(ReadOnlyTargetRules Target) : base(Target)
	{
		// Autogenerate headers for our bytecode.
		GenerateHeaderFuncs.Add(("VerseVMBytecode", VerseVMBytecodeGenerator.Generate));

		PrivatePCHHeaderFile = "Private/CoreUObjectPrivatePCH.h";

		SharedPCHHeaderFile = "Public/CoreUObjectSharedPCH.h";

		PrivateIncludePathModuleNames.AddRange(
			new string[]
			{
				"TargetPlatform",
			}
		);

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"TraceLog",
				"CorePreciseFP",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",
				"Json",
			}
		);

		AddVerseVMDependencies(this, Target);

		if (Target.bBuildWithEditorOnlyData || Target.Type == TargetType.Server)
		{
			PublicDefinitions.Add("WITH_VERSE_COMPILER=1");
		}
		else
		{
			PublicDefinitions.Add("WITH_VERSE_COMPILER=0");
		}

		UnsafeTypeCastWarningLevel = WarningLevel.Error;

		if (Target.bBuildWithEditorOnlyData)
		{
			PrivateDependencyModuleNames.Add("DerivedDataCache");
		}

		PrivateDefinitions.Add("UE_DEFINE_LEGACY_MATH_CONSTANT_MACRO_NAMES=0");
	}
}
