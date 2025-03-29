#pragma once

#include "DataDrivenShaderPlatformInfo.h"
#include "MeshPassProcessor.h"
#include "InstanceCulling/InstanceCullingContext.h"

struct FFastVramConfig;

class FToonMainLightMeshProcessor : public FMeshPassProcessor
{
public:
	FToonMainLightMeshProcessor(
		const FScene* Scene,
		const FSceneView* InViewIfDynamicMeshCommand,
		const FMeshPassProcessorRenderState& InPassDrawRenderState,
		FMeshPassDrawListContext* InDrawListContext
	);
	
private:
	// 准备好数据(MeshBatch，要用什么shader绘制，shader参数，剔除方式，深度测试等)
	// 将数据传递给BuildMeshDrawCommands生成MeshDrawCommand
	// MeshDrawCommand是完整描述了一个Pass Draw Call的所有状态和数据，如shader绑定、顶点数据、索引数据、PSO缓存等
	// 之后引擎会把MeshDrawCommand转化为RHI命令进行渲染
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
	
	virtual void CollectPSOInitializers(
		const FSceneTexturesConfig& SceneTexturesConfig,
		const FMaterial& Material,
		const FPSOPrecacheVertexFactoryData& VertexFactoryData,
		const FPSOPrecacheParams& PreCacheParams,
		TArray<FPSOPrecacheData>& PSOInitializers
	) override;

	bool TryAddMeshBatch(
		const FMeshBatch& RESTRICT MeshBatch,
		uint64 BatchElementMask,
		const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy,
		int32 StaticMeshId,
		const FMaterialRenderProxy& MaterialRenderProxy,
		const FMaterial& Material);
	
	// 函数将会从引擎底层拿到MeshBatch，Material等资源
	// MeshBatch简单理解就是同一批次的网格
	// 我们通过这个函数筛选哪些Mesh需要绘制并调用Process()
	virtual void AddMeshBatch(
		const FMeshBatch& RESTRICT MeshBatch,
		uint64 BatchElementMask,
		const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy,
		int32 StaticMeshId = -1
	) override final;
	
	FMeshPassProcessorRenderState PassDrawRenderState;
};

class FToonLightPassVS : public FMeshMaterialShader
{
    DECLARE_SHADER_TYPE(FToonLightPassVS, MeshMaterial);

public:
    FToonLightPassVS() = default;
    FToonLightPassVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
        : FMeshMaterialShader(Initializer)
    {
    }

    static void ModifyCompilationEnvironment(const FMaterialShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
    {
    	FMeshMaterialShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
    	OutEnvironment.SetDefine(TEXT("TOONLIGHTING"), 1);
    }

	static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters)
    {
    	return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5) &&
			(Parameters.VertexFactoryType->GetFName() == FName(TEXT("FLocalVertexFactory")) || 
				Parameters.VertexFactoryType->GetFName() == FName(TEXT("TGPUSkinVertexFactoryDefault")));
    }

};


class FToonLightPassPS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FToonLightPassPS, MeshMaterial);
public:
    FToonLightPassPS() = default;
    FToonLightPassPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
        : FMeshMaterialShader(Initializer)
    {
    }

    static void ModifyCompilationEnvironment(const FMaterialShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
    {
    	FMeshMaterialShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
    	OutEnvironment.SetDefine(TEXT("TOONLIGHTING"), 1);
    }

	static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters)
    {
    	return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5) &&
			(Parameters.VertexFactoryType->GetFName() == FName(TEXT("FLocalVertexFactory")) || 
				Parameters.VertexFactoryType->GetFName() == FName(TEXT("TGPUSkinVertexFactoryDefault")));
    }

    void GetShaderBindings(
        const FScene* Scene,
        ERHIFeatureLevel::Type FeatureLevel,
        const FPrimitiveSceneProxy* PrimitiveSceneProxy,
        const FMaterialRenderProxy& MaterialRenderProxy,
        const FMaterial& Material,
        const FMeshMaterialShaderElementData& ShaderElementData,
        FMeshDrawSingleShaderBindings& ShaderBindings) const
    {
        FMeshMaterialShader::GetShaderBindings(Scene, FeatureLevel, PrimitiveSceneProxy, MaterialRenderProxy, Material, ShaderElementData, ShaderBindings);
    }
};

FRDGTextureDesc GetToonShadowTextureDesc(FIntPoint Extent, ETextureCreateFlags CreateFlags);
FRDGTextureRef CreateToonShadowTexture(FRDGBuilder& GraphBuilder, FIntPoint Extent, ETextureCreateFlags CreateFlags, const TCHAR* Name);