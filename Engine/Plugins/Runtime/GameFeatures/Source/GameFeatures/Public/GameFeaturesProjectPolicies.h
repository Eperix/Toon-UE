// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UObject/PrimaryAssetId.h"
#include "GameFeaturesSubsystem.h"

#include "GameFeaturesProjectPolicies.generated.h"

class UGameFeatureData;

// This class allows project-specific rules to be implemented for game feature plugins.
// Create a subclass and choose it in Project Settings .. Game Features
UCLASS()
class GAMEFEATURES_API UGameFeaturesProjectPolicies : public UObject
{
	GENERATED_BODY()

public:
	// Called when the game feature manager is initialized
	virtual void InitGameFeatureManager() { }

	// Called when the game feature manager is shut down
	virtual void ShutdownGameFeatureManager() { }

	// Called to determined the expected state of a plugin under the WhenLoading conditions.
	virtual bool WillPluginBeCooked(const FString& PluginFilename, const FGameFeaturePluginDetails& PluginDetails) const;

	// Called when a game feature plugin enters the Loading state to determine additional assets to load
	virtual TArray<FPrimaryAssetId> GetPreloadAssetListForGameFeature(const UGameFeatureData* GameFeatureToLoad, bool bIncludeLoadedAssets = false) const { return TArray<FPrimaryAssetId>(); }

	// Returns the bundle state to use for assets returned by GetPreloadAssetListForGameFeature()
	// See the Asset Manager documentation for more information about asset bundles
	virtual const TArray<FName> GetPreloadBundleStateForGameFeature() const { return TArray<FName>(); }

	// Called to determine if this should be treated as a client, server, or both for data preloading
	// Actions can use this to decide what to load at runtime
	virtual void GetGameFeatureLoadingMode(bool& bLoadClientData, bool& bLoadServerData) const { bLoadClientData = true; bLoadServerData = true; }

	// Called to determine the plugin URL for a given known Plugin. Can be used if the policy wants to deliver non file based URLs.
	virtual bool GetGameFeaturePluginURL(const TSharedRef<IPlugin>& Plugin, FString& OutPluginURL) const;

	// Called to determine if a plugin is allowed to be loaded or not
	// (e.g., when doing a fast cook a game might want to disable some or all game feature plugins)
	virtual bool IsPluginAllowed(const FString& PluginURL) const { return true; }

	// Called to resolve plugin dependencies, will successfully return an empty string if a dependency is not a GFP.
	// This may be called with file protocol for built-in plugins in some cases, even if a different protocol is used at runtime.
	// returns The dependency URL or an error if the dependency could not be resolved
	virtual TValueOrError<FString, FString> ResolvePluginDependency(const FString& PluginURL, const FString& DependencyName) const;

	// Called to resolve install bundles for streaming asset dependencies
	virtual TValueOrError<TArray<FName>, FString> GetStreamingAssetInstallBundles(FStringView PluginURL) const { return MakeValue(); }

	// Called by code that explicitly wants to load a specific plugin
	// (e.g., when using a fast cook a game might want to allow explicitly loaded game feature plugins)
	virtual void ExplicitLoadGameFeaturePlugin(const FString& PluginURL, const FGameFeaturePluginLoadComplete& CompleteDelegate, const bool bActivateGameFeatures);
};

// This is a default implementation that immediately processes all game feature plugins the based on
// their BuiltInAutoRegister, BuiltInAutoLoad, and BuiltInAutoActivate settings.
//
// It will be used if no project-specific policy is set in Project Settings .. Game Features
UCLASS()
class GAMEFEATURES_API UDefaultGameFeaturesProjectPolicies : public UGameFeaturesProjectPolicies
{
	GENERATED_BODY()

public:
	//~UGameFeaturesProjectPolicies interface
	virtual void InitGameFeatureManager() override;
	virtual void GetGameFeatureLoadingMode(bool& bLoadClientData, bool& bLoadServerData) const override;
	virtual const TArray<FName> GetPreloadBundleStateForGameFeature() const override;
	//~End of UGameFeaturesProjectPolicies interface
};

#if UE_ENABLE_INCLUDE_ORDER_DEPRECATED_IN_5_2
#include "CoreMinimal.h"
#endif
