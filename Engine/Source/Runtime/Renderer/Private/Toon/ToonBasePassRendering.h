#pragma once
#include "MeshPassProcessor.h"
#include "MeshMaterialShader.h"
#include "DataDrivenShaderPlatformInfo.h"

class FToonBasePassProcessor : public FMeshPassProcessor
{
	public:
	FToonBasePassProcessor(
		const FScene* Scene,  
		const FSceneView* InViewIfDynamicMeshCommand,  
		const FMeshPassProcessorRenderState& InPassDrawRenderState,  
		FMeshPassDrawListContext* InDrawListContext
	);

	// AddMeshBatch函数从底层拿到MeshBatch、Material等资源，然后通过AddMeshBatch函数筛选需要绘制的Mesh并调用Process
	virtual void AddMeshBatch(const FMeshBatch& RESTRICT MeshBatch,
		uint64 BatchElementMask,
		const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy,
		int32 StaticMeshId = -1) override final;

	private:
	// 准备好数据(MeshBatch，要用什么shader绘制，shader参数，剔除方式，深度测试等)，传递给BuildMeshDrawCommand生成MeshDrawCommand
	// 之后引擎会把DrawCommand转化为RHI进行渲染
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

// Shader Start
class FToonBasePassVS : public FMeshMaterialShader
{
	// 声明Shader，实际上是初始化各种指针以及类型“MeshMaterial”
	DECLARE_SHADER_TYPE(FToonBasePassVS, MeshMaterial);
public:
	FToonBasePassVS() = default;
	FToonBasePassVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):FMeshMaterialShader(Initializer)
	{}
	// 修改Shader环境
	static void ModifyCompilationEnvironment(const FMaterialShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{}

	// 根据平台/数据决定是否应该编译
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

class FToonBasePassPS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FToonBasePassPS, MeshMaterial);

public:
	FToonBasePassPS() = default;
	FToonBasePassPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):FMeshMaterialShader(Initializer)
	{}
	static void ModifyCompilationEnvironment(const FMaterialShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{}

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

		FVector3f Color(1.0, 0.0, 0.0);

		ShaderBindings.Add(InputColor, Color);
	}

	LAYOUT_FIELD(FShaderParameter, InputColor);
};

//Shader End

FRDGTextureDesc GetToonBufferTextureDesc(FIntPoint Extent, ETextureCreateFlags CreateFlags);
FRDGTextureRef CreateToonBufferTexture(FRDGBuilder& GraphBuilder, FIntPoint Extent, ETextureCreateFlags CreateFlags, const TCHAR* Name);