// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InterchangeMeshFactoryNode.h"
#include "Nodes/InterchangeFactoryBaseNode.h"

#if WITH_ENGINE
#include "Engine/StaticMesh.h"
#endif

#include "InterchangeStaticMeshFactoryNode.generated.h"


namespace UE
{
	namespace Interchange
	{
		struct INTERCHANGEFACTORYNODES_API FStaticMeshNodeStaticData : public FBaseNodeStaticData
		{
			static const FAttributeKey& GetLODScreenSizeBaseKey();
			static const FAttributeKey& GetSocketUidsBaseKey();
		};
	} // namespace Interchange
} // namespace UE


UCLASS(BlueprintType)
class INTERCHANGEFACTORYNODES_API UInterchangeStaticMeshFactoryNode : public UInterchangeMeshFactoryNode
{
	GENERATED_BODY()

public:
	UInterchangeStaticMeshFactoryNode();

	/**
	 * Initialize node data.
	 * @param UniqueID - The unique ID for this node.
	 * @param DisplayLabel - The name of the node.
	 * @param InAssetClass - The class the StaticMesh factory will create for this node.
	 */
	UFUNCTION(BlueprintCallable, Category = "Interchange | Node | StaticMesh")
	void InitializeStaticMeshNode(const FString& UniqueID, const FString& DisplayLabel, const FString& InAssetClass);

	/**
	 * Return the node type name of the class. This is used when reporting errors.
	 */
	virtual FString GetTypeName() const override;

	/** Get the class this node creates. */
	UFUNCTION(BlueprintCallable, Category = "Interchange | Node | StaticMesh")
	virtual class UClass* GetObjectClass() const override;

#if WITH_EDITOR

	virtual FString GetKeyDisplayName(const UE::Interchange::FAttributeKey& NodeAttributeKey) const override;
	virtual FString GetAttributeCategory(const UE::Interchange::FAttributeKey& NodeAttributeKey) const override;

#endif //WITH_EDITOR

public:
	/** Get whether the static mesh factory should auto compute LOD Screen Sizes. Return false if the attribute was not set. */
	UFUNCTION(BlueprintCallable, Category = "Interchange | Node | StaticMesh")
	bool GetCustomAutoComputeLODScreenSizes(bool& AttributeValue) const;

	/** Set whether the static mesh factory should auto compute LOD Screen Sizes. Return false if the attribute was not set. */
	UFUNCTION(BlueprintCallable, Category = "Interchange | Node | StaticMesh")
	bool SetCustomAutoComputeLODScreenSizes(const bool& AttributeValue);

	/** Returns the number of LOD Screen Sizes the static mesh has.*/
	UFUNCTION(BlueprintCallable, Category = "Interchange | Node | StaticMesh")
	int32 GetLODScreenSizeCount() const;

	/** Returns All the LOD Screen Sizes set for the static mesh.*/
	UFUNCTION(BlueprintCallable, Category = "Interchange | Node | StaticMesh")
	void GetLODScreenSizes(TArray<float>& OutLODScreenSizes) const;

	/** Sets the LOD Screen Sizes for the static mesh.*/
	UFUNCTION(BlueprintCallable, Category = "Interchange | Node | StaticMesh")
	bool SetLODScreenSizes(const TArray<float>& InLODScreenSizes);

	/** Get whether the static mesh factory should set the Nanite build setting. Return false if the attribute was not set. */
	UFUNCTION(BlueprintCallable, Category = "Interchange | Node | StaticMesh")
	bool GetCustomBuildNanite(bool& AttributeValue) const;

	/** Set whether the static mesh factory should set the Nanite build setting. Return false if the attribute was not set. */
	UFUNCTION(BlueprintCallable, Category = "Interchange | Node | StaticMesh")
	bool SetCustomBuildNanite(const bool& AttributeValue, bool bAddApplyDelegate = true);

	/** Return the number of socket UIDs this static mesh has. */
	UFUNCTION(BlueprintCallable, Category = "Interchange | Node | StaticMesh")
	int32 GetSocketUidCount() const;

	UFUNCTION(BlueprintCallable, Category = "Interchange | Node | StaticMesh")
	void GetSocketUids(TArray<FString>& OutSocketUids) const;

	UFUNCTION(BlueprintCallable, Category = "Interchange | Node | StaticMesh")
	bool AddSocketUid(const FString& SocketUid);

	UFUNCTION(BlueprintCallable, Category = "Interchange | Node | StaticMesh")
	bool AddSocketUids(const TArray<FString>& InSocketUids);

	UFUNCTION(BlueprintCallable, Category = "Interchange | Node | StaticMesh")
	bool RemoveSocketUd(const FString& SocketUid);

	/** Get whether the static mesh should build a reversed index buffer. */
	UFUNCTION(BlueprintCallable, Category = "Interchange | Node | StaticMesh")
	bool GetCustomBuildReversedIndexBuffer(bool& AttributeValue) const;

	/** Set whether the static mesh should build a reversed index buffer. */
	UFUNCTION(BlueprintCallable, Category = "Interchange | Node | StaticMesh")
	bool SetCustomBuildReversedIndexBuffer(const bool& AttributeValue, bool bAddApplyDelegate = true);

	/** Get whether the static mesh should generate lightmap UVs. */
	UFUNCTION(BlueprintCallable, Category = "Interchange | Node | StaticMesh")
	bool GetCustomGenerateLightmapUVs(bool& AttributeValue) const;

	/** Set whether the static mesh should generate lightmap UVs. */
	UFUNCTION(BlueprintCallable, Category = "Interchange | Node | StaticMesh")
	bool SetCustomGenerateLightmapUVs(const bool& AttributeValue, bool bAddApplyDelegate = true);

	/**
	 * Get whether to generate the distance field by treating every triangle hit as a front face.  
	 * This prevents the distance field from being discarded due to the mesh being open, but also lowers distance field ambient occlusion quality.
	 */
	UFUNCTION(BlueprintCallable, Category = "Interchange | Node | StaticMesh")
	bool GetCustomGenerateDistanceFieldAsIfTwoSided(bool& AttributeValue) const;

	/**
	 * Set whether to generate the distance field by treating every triangle hit as a front face.
	 * This prevents the distance field from being discarded due to the mesh being open, but also lowers distance field ambient occlusion quality.
	 */
	UFUNCTION(BlueprintCallable, Category = "Interchange | Node | StaticMesh")
	bool SetCustomGenerateDistanceFieldAsIfTwoSided(const bool& AttributeValue, bool bAddApplyDelegate = true);

	/** Get whether the static mesh is set up for use with physical material masks. */
	UFUNCTION(BlueprintCallable, Category = "Interchange | Node | StaticMesh")
	bool GetCustomSupportFaceRemap(bool& AttributeValue) const;

	/** Set whether the static mesh is set up for use with physical material masks. */
	UFUNCTION(BlueprintCallable, Category = "Interchange | Node | StaticMesh")
	bool SetCustomSupportFaceRemap(const bool& AttributeValue, bool bAddApplyDelegate = true);

	/** Get the amount of padding used to pack UVs for the static mesh. */
	UFUNCTION(BlueprintCallable, Category = "Interchange | Node | StaticMesh")
	bool GetCustomMinLightmapResolution(int32& AttributeValue) const;

	/** Set the amount of padding used to pack UVs for the static mesh. */
	UFUNCTION(BlueprintCallable, Category = "Interchange | Node | StaticMesh")
	bool SetCustomMinLightmapResolution(const int32& AttributeValue, bool bAddApplyDelegate = true);

	/** Get the index of the UV that is used as the source for generating lightmaps for the static mesh.  */
	UFUNCTION(BlueprintCallable, Category = "Interchange | Node | StaticMesh")
	bool GetCustomSrcLightmapIndex(int32& AttributeValue) const;

	/** Set the index of the UV that is used as the source for generating lightmaps for the static mesh. */
	UFUNCTION(BlueprintCallable, Category = "Interchange | Node | StaticMesh")
	bool SetCustomSrcLightmapIndex(const int32& AttributeValue, bool bAddApplyDelegate = true);

	/** Get the index of the UV that is used to store generated lightmaps for the static mesh. */
	UFUNCTION(BlueprintCallable, Category = "Interchange | Node | StaticMesh")
	bool GetCustomDstLightmapIndex(int32& AttributeValue) const;

	/** Set the index of the UV that is used to store generated lightmaps for the static mesh. */
	UFUNCTION(BlueprintCallable, Category = "Interchange | Node | StaticMesh")
	bool SetCustomDstLightmapIndex(const int32& AttributeValue, bool bAddApplyDelegate = true);

	/** Get the local scale that is applied when building the static mesh. */
	UFUNCTION(BlueprintCallable, Category = "Interchange | Node | StaticMesh")
	bool GetCustomBuildScale3D(FVector& AttributeValue) const;

	/** Set the local scale that is applied when building the static mesh. */
	UFUNCTION(BlueprintCallable, Category = "Interchange | Node | StaticMesh")
	bool SetCustomBuildScale3D(const FVector& AttributeValue, bool bAddApplyDelegate = true);

	/**
	 * Get the scale to apply to the mesh when allocating the distance field volume texture.
	 * The default scale is 1, which assumes that the mesh will be placed unscaled in the world.
	 */
	UFUNCTION(BlueprintCallable, Category = "Interchange | Node | StaticMesh")
	bool GetCustomDistanceFieldResolutionScale(float& AttributeValue) const;

	/**
	 * Set the scale to apply to the mesh when allocating the distance field volume texture.
	 * The default scale is 1, which assumes that the mesh will be placed unscaled in the world.
	 */
	UFUNCTION(BlueprintCallable, Category = "Interchange | Node | StaticMesh")
	bool SetCustomDistanceFieldResolutionScale(const float& AttributeValue, bool bAddApplyDelegate = true);

	/** Get the static mesh asset whose distance field will be used as the distance field for the imported mesh. */
	UFUNCTION(BlueprintCallable, Category = "Interchange | Node | StaticMesh")
	bool GetCustomDistanceFieldReplacementMesh(FSoftObjectPath& AttributeValue) const;

	/** Set the static mesh asset whose distance field will be used as the distance field for the imported mesh. */
	UFUNCTION(BlueprintCallable, Category = "Interchange | Node | StaticMesh")
	bool SetCustomDistanceFieldReplacementMesh(const FSoftObjectPath& AttributeValue, bool bAddApplyDelegate = true);

	/**
	 * Get the maximum number of Lumen mesh cards to generate for this mesh.
	 * More cards means that the surface will have better coverage, but will result in increased runtime overhead.
	 * Set this to 0 to disable mesh card generation for this mesh.
	 * The default is 12.
	 */
	UFUNCTION(BlueprintCallable, Category = "Interchange | Node | StaticMesh")
	bool GetCustomMaxLumenMeshCards(int32& AttributeValue) const;

	/**
	 * Set the maximum number of Lumen mesh cards to generate for this mesh.
	 * More cards means that the surface will have better coverage, but will result in increased runtime overhead.
	 * Set this to 0 to disable mesh card generation for this mesh.
	 * The default is 12.
	 */
	UFUNCTION(BlueprintCallable, Category = "Interchange | Node | StaticMesh")
	bool SetCustomMaxLumenMeshCards(const int32& AttributeValue, bool bAddApplyDelegate = true);

private:
	virtual void FillAssetClassFromAttribute() override;
	virtual bool SetNodeClassFromClassAttribute() override;

	const UE::Interchange::FAttributeKey Macro_CustomBuildReversedIndexBufferKey = UE::Interchange::FAttributeKey(TEXT("BuildReversedIndexBuffer"));
	const UE::Interchange::FAttributeKey Macro_CustomGenerateLightmapUVsKey = UE::Interchange::FAttributeKey(TEXT("GenerateLightmapUVs"));
	const UE::Interchange::FAttributeKey Macro_CustomGenerateDistanceFieldAsIfTwoSidedKey = UE::Interchange::FAttributeKey(TEXT("GenerateDistanceFieldAsIfTwoSided"));
	const UE::Interchange::FAttributeKey Macro_CustomSupportFaceRemapKey = UE::Interchange::FAttributeKey(TEXT("SupportFaceRemap"));
	const UE::Interchange::FAttributeKey Macro_CustomMinLightmapResolutionKey = UE::Interchange::FAttributeKey(TEXT("MinLightmapResolution"));
	const UE::Interchange::FAttributeKey Macro_CustomSrcLightmapIndexKey = UE::Interchange::FAttributeKey(TEXT("SrcLightmapIndex"));
	const UE::Interchange::FAttributeKey Macro_CustomDstLightmapIndexKey = UE::Interchange::FAttributeKey(TEXT("DstLightmapIndex"));
	const UE::Interchange::FAttributeKey Macro_CustomBuildScale3DKey = UE::Interchange::FAttributeKey(TEXT("BuildScale3D"));
	const UE::Interchange::FAttributeKey Macro_CustomDistanceFieldResolutionScaleKey = UE::Interchange::FAttributeKey(TEXT("DistanceFieldResolutionScale"));
	const UE::Interchange::FAttributeKey Macro_CustomDistanceFieldReplacementMeshKey = UE::Interchange::FAttributeKey(TEXT("DistanceFieldReplacementMesh"));
	const UE::Interchange::FAttributeKey Macro_CustomMaxLumenMeshCardsKey = UE::Interchange::FAttributeKey(TEXT("MaxLumenMeshCards"));
	const UE::Interchange::FAttributeKey Macro_CustomBuildNaniteKey = UE::Interchange::FAttributeKey(TEXT("BuildNanite"));
	const UE::Interchange::FAttributeKey Macro_CustomAutoComputeLODScreenSizesKey = UE::Interchange::FAttributeKey(TEXT("AutoComputeLODScreenSizes"));


	UE::Interchange::TArrayAttributeHelper<float> LODScreenSizes;
	UE::Interchange::TArrayAttributeHelper<FString> SocketUids;

protected:
	
#if WITH_EDITORONLY_DATA
	IMPLEMENT_NODE_ATTRIBUTE_DELEGATE_BY_PROPERTYNAME(BuildNanite, bool, UStaticMesh, TEXT("NaniteSettings.bEnabled"));
#endif

	bool ApplyCustomBuildReversedIndexBufferToAsset(UObject* Asset) const;
	bool FillCustomBuildReversedIndexBufferFromAsset(UObject* Asset);
	bool ApplyCustomGenerateLightmapUVsToAsset(UObject* Asset) const;
	bool FillCustomGenerateLightmapUVsFromAsset(UObject* Asset);
	bool ApplyCustomGenerateDistanceFieldAsIfTwoSidedToAsset(UObject* Asset) const;
	bool FillCustomGenerateDistanceFieldAsIfTwoSidedFromAsset(UObject* Asset);
	bool ApplyCustomSupportFaceRemapToAsset(UObject* Asset) const;
	bool FillCustomSupportFaceRemapFromAsset(UObject* Asset);
	bool ApplyCustomMinLightmapResolutionToAsset(UObject* Asset) const;
	bool FillCustomMinLightmapResolutionFromAsset(UObject* Asset);
	bool ApplyCustomSrcLightmapIndexToAsset(UObject* Asset) const;
	bool FillCustomSrcLightmapIndexFromAsset(UObject* Asset);
	bool ApplyCustomDstLightmapIndexToAsset(UObject* Asset) const;
	bool FillCustomDstLightmapIndexFromAsset(UObject* Asset);
	bool ApplyCustomBuildScale3DToAsset(UObject* Asset) const;
	bool FillCustomBuildScale3DFromAsset(UObject* Asset);
	bool ApplyCustomDistanceFieldResolutionScaleToAsset(UObject* Asset) const;
	bool FillCustomDistanceFieldResolutionScaleFromAsset(UObject* Asset);
	bool ApplyCustomDistanceFieldReplacementMeshToAsset(UObject* Asset) const;
	bool FillCustomDistanceFieldReplacementMeshFromAsset(UObject* Asset);
	bool ApplyCustomMaxLumenMeshCardsToAsset(UObject* Asset) const;
	bool FillCustomMaxLumenMeshCardsFromAsset(UObject* Asset);

#if WITH_ENGINE
	TSubclassOf<UStaticMesh> AssetClass = nullptr;
#endif
};
