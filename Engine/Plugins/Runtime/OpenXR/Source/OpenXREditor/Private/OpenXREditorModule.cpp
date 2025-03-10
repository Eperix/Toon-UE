// Copyright Epic Games, Inc. All Rights Reserved.

#include "OpenXREditorModule.h"
#include "OpenXRAssetDirectory.h"
#include "OpenXRHMDSettings.h"

#include "ISettingsModule.h"
#include "Modules/ModuleManager.h"

#include "Editor/EditorPerformanceSettings.h"

#define LOCTEXT_NAMESPACE "OpenXR"

void FOpenXREditorModule::StartupModule()
{
	FOpenXRAssetDirectory::LoadForCook();

	// register settings
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");

	if (SettingsModule != nullptr)
	{
		SettingsModule->RegisterSettings("Project", "Plugins", "OpenXRHMD",
			LOCTEXT("OpenXRHMDSettingsName", "OpenXR Settings"),
			LOCTEXT("OpenXRHMDSettingsDescription", "Project settings for OpenXR plugin"),
			GetMutableDefault<UOpenXRHMDSettings>()
		);
	}

	UEditorPerformanceSettings* EditorPerformanceSettings = GetMutableDefault<UEditorPerformanceSettings>();
	if (EditorPerformanceSettings->bOverrideMaxViewportRenderingResolution)
	{
		UE_LOG(LogTemp, Warning, TEXT("Existing value for UEditorPerformanceSettings::MaxViewportRenderingResolution will be overriden for OpenXR."));
	}

	UE_LOG(LogTemp, Log, TEXT("OpenXR ignores max viewport resolution in editor to support full HMD resolutions."));
	EditorPerformanceSettings->bOverrideMaxViewportRenderingResolution = true;
	EditorPerformanceSettings->MaxViewportRenderingResolution = 0;

	FPropertyChangedEvent DisabledMaxResolutionEvent(EditorPerformanceSettings->GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UEditorPerformanceSettings, MaxViewportRenderingResolution)), EPropertyChangeType::ValueSet);
	EditorPerformanceSettings->PostEditChangeProperty(DisabledMaxResolutionEvent);
}

void FOpenXREditorModule::ShutdownModule()
{
	FOpenXRAssetDirectory::ReleaseAll();

	// unregister settings
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");

	if (SettingsModule != nullptr)
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "OpenXR");
		SettingsModule->UnregisterSettings("Project", "Plugins", "OpenXRHMD");
	}
}

IMPLEMENT_MODULE(FOpenXREditorModule, OpenXREditor);

#undef LOCTEXT_NAMESPACE
