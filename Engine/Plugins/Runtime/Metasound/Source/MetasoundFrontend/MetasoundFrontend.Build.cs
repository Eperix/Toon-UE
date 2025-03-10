// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

namespace UnrealBuildTool.Rules
{
	public class MetasoundFrontend : ModuleRules
	{
		public MetasoundFrontend(ReadOnlyTargetRules Target) : base(Target)
		{
			NumIncludedBytesPerUnityCPPOverride = 294912; // best unity size found from using UBT ProfileUnitySizes mode

			PublicDependencyModuleNames.AddRange
			(
				new string[]
				{
					"AudioExtensions",
					"Core",
					"CoreUObject",
					"Serialization",
					"SignalProcessing",
					"MetasoundGraphCore"
				}
			);

			PublicDefinitions.Add("WITH_METASOUND_FRONTEND=1");

			bDisableAutoRTFMInstrumentation = true;
		}
	}
}
