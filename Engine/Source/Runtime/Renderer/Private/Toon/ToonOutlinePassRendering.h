#pragma once
#include "MeshPassProcessor.h"

class FToonOutlinePassProcessor : public FMeshPassProcessor
{
public:  
	//const FScene* InScene, ERHIFeatureLevel::Type InFeatureLevel, const FSceneView* InViewIfDynamicMeshCommand, FMeshPassDrawListContext* InDrawListContext  
	FToonOutlinePassProcessor(  
	   const FScene* Scene,  
	   const FSceneView* InViewIfDynamicMeshCommand,  
	   const FMeshPassProcessorRenderState& InPassDrawRenderState,  
	   FMeshPassDrawListContext* InDrawListContext  
	);  
	virtual void AddMeshBatch(  
	   const FMeshBatch& RESTRICT MeshBatch,  
	   uint64 BatchElementMask,  
	   const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy,  
	   int32 StaticMeshId = -1  
	) override final;
	
private:
	bool Process(  
		   const FMeshBatch& MeshBatch,  
		   uint64 BatchElementMask,  
		   int32 StaticMeshId,  
		   const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy,  
		   const FMaterialRenderProxy& RESTRICT MaterialRenderProxy,  
		   const FMaterial& RESTRICT MaterialResource,  
		   ERasterizerFillMode MeshFillMode,  
		   ERasterizerCullMode MeshCullMode  
	);  
	FMeshPassProcessorRenderState PassDrawRenderState; 
};