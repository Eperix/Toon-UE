// Copyright Epic Games, Inc. All Rights Reserved.

#include "ToonPostProcess.h"

#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FToonPostProcessModule"

void FToonPostProcessModule::StartupModule()
{
	FString ShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("ToonPostProcess"))->GetBaseDir(), TEXT("/Shaders"));
	AddShaderSourceDirectoryMapping("/Project/Shaders", ShaderDir);
}

void FToonPostProcessModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FToonPostProcessModule, ToonPostProcess)