#include "ToonBasePassRendering.h"

#include "ScenePrivate.h"
#include "MeshPassProcessor.inl"
#include "SimpleMeshDrawCommandPass.h"
#include "StaticMeshBatch.h"
#include "DeferredShadingRenderer.h"
#include "ClearQuad.h"
#include "PipelineFileCache.h"

// IMPLEMENT_MATERIAL_SHADER_TYPE接受的参数：
// FToonBasePassPS:我们在ToonPassRendering.h中定义的shader类
// TEXT("/Engine/Private/Toon/ToonPassShader.usf"):我们使用的shader路径
// TEXT("MainPS"):shader的入口函数名
// SF_Pixel:shader的类型，Vertex shader、Pixel shader或者compute shader
IMPLEMENT_MATERIAL_SHADER_TYPE(, FToonBasePassVS, TEXT("/Engine/Private/Toon/ToonMeshPassVS.usf"), TEXT("MainVS"), SF_Vertex);
IMPLEMENT_MATERIAL_SHADER_TYPE(, FToonBasePassPS, TEXT("/Engine/Private/Toon/ToonBasePassPS.usf"), TEXT("MainPS"), SF_Pixel);

//--------------------------------------------Toon Buffer Texture---------------------------------------------
// Toon Buffer step 5-2
void CreateToonBuffers(FRDGBuilder& GraphBuilder, FSceneTextures& SceneTexture, FIntPoint Extent, const FFastVramConfig& FastVRamConfig)
{
	const FRDGTextureDesc TBufferADesc = FRDGTextureDesc(FRDGTextureDesc::Create2D(Extent, PF_R8G8B8A8_UINT,
		FClearValueBinding::Black, TexCreate_UAV | TexCreate_RenderTargetable | TexCreate_ShaderResource | FastVRamConfig.TBufferA));
	SceneTexture.TBufferA = GraphBuilder.CreateTexture(TBufferADesc, TEXT("TBufferA"));

	const FRDGTextureDesc TBufferBDesc = FRDGTextureDesc(FRDGTextureDesc::Create2D(Extent, PF_R8G8B8A8,
		FClearValueBinding::Black, TexCreate_UAV | TexCreate_RenderTargetable | TexCreate_ShaderResource | FastVRamConfig.TBufferB));
	SceneTexture.TBufferB = GraphBuilder.CreateTexture(TBufferBDesc, TEXT("TBufferB"));

	const FRDGTextureDesc TBufferCDesc = FRDGTextureDesc(FRDGTextureDesc::Create2D(Extent, PF_R8G8B8A8,
		FClearValueBinding::Black, TexCreate_UAV | TexCreate_RenderTargetable | TexCreate_ShaderResource | FastVRamConfig.TBufferC));
	SceneTexture.TBufferC = GraphBuilder.CreateTexture(TBufferCDesc, TEXT("TBufferC"));

}

 //------------------------------------------- Mesh Pass Processor-------------------------------------------------

FToonBasePassProcessor::FToonBasePassProcessor(
	const FScene* Scene,  
	const FSceneView* InViewIfDynamicMeshCommand,  
	const FMeshPassProcessorRenderState& InPassDrawRenderState,  
	FMeshPassDrawListContext* InDrawListContext )
:FMeshPassProcessor(EMeshPass::ToonBasePass, Scene, Scene->GetFeatureLevel(), InViewIfDynamicMeshCommand, InDrawListContext),
PassDrawRenderState(InPassDrawRenderState)
{
	// 设置默认的BlendState和DepthStencilState
	// BlendState控制颜色混合方式
	// DepthStencilState控制深度写入，深度测试等行为
    if (PassDrawRenderState.GetDepthStencilState() == nullptr)
    {
    	PassDrawRenderState.SetDepthStencilState(TStaticDepthStencilState<false, CF_DepthNearOrEqual>().GetRHI());
    }
    if (PassDrawRenderState.GetBlendState() == nullptr)
    {
        PassDrawRenderState.SetBlendState(TStaticBlendState<>().GetRHI());
    }
}

void FToonBasePassProcessor::AddMeshBatch(
    const FMeshBatch& MeshBatch,
    uint64 BatchElementMask,
    const FPrimitiveSceneProxy* PrimitiveSceneProxy,
    int32 StaticMeshId)
{
    const FMaterialRenderProxy* MaterialRenderProxy = MeshBatch.MaterialRenderProxy;

    const FMaterial* Material = MaterialRenderProxy->GetMaterialNoFallback(FeatureLevel);
	
    if (Material != nullptr && Material->GetRenderingThreadShaderMap())
    {
    	const FMaterialShadingModelField ShadingModels = Material->GetShadingModels();
    	// 只有材质使用了Toon相关的shading model才会被绘制
	    if (ShadingModels.HasShadingModel(MSM_ToonDefault) || ShadingModels.HasShadingModel(MSM_ToonHair))
	    {
	    	const EBlendMode BlendMode = Material->GetBlendMode();

	    	bool bResult = true;
	    	if (BlendMode == BLEND_Opaque || BlendMode == SE_BLEND_Masked)
	    	{
	    		Process(
					MeshBatch,
					BatchElementMask,
					StaticMeshId,
					PrimitiveSceneProxy,
					*MaterialRenderProxy,
					*Material,
					FM_Solid,
					CM_CW); //背面剔除
	    	}
	    }
    }
}

bool FToonBasePassProcessor::Process(
    const FMeshBatch& MeshBatch,
    uint64 BatchElementMask,
    int32 StaticMeshId,
    const FPrimitiveSceneProxy* PrimitiveSceneProxy,
    const FMaterialRenderProxy& MaterialRenderProxy,
    const FMaterial& RESTRICT MaterialResource,
    ERasterizerFillMode MeshFillMode,
    ERasterizerCullMode MeshCullMode)
{
    const FVertexFactory* VertexFactory = MeshBatch.VertexFactory;

    TMeshProcessorShaders<FToonBasePassVS, FToonBasePassPS> ToonBasePassShaders;
    {
        FMaterialShaderTypes ShaderTypes;
    	// 指定使用的shader
        ShaderTypes.AddShaderType<FToonBasePassVS>();
        ShaderTypes.AddShaderType<FToonBasePassPS>();

        const FVertexFactoryType* VertexFactoryType = VertexFactory->GetType();

        FMaterialShaders Shaders;
        if (!MaterialResource.TryGetShaders(ShaderTypes, VertexFactoryType, Shaders))
        {
            //UE_LOG(LogShaders, Warning, TEXT("Shader Not Found!"));
            return false;
        }

        Shaders.TryGetVertexShader(ToonBasePassShaders.VertexShader);
        Shaders.TryGetPixelShader(ToonBasePassShaders.PixelShader);
    }


    FMeshMaterialShaderElementData ShaderElementData;
    ShaderElementData.InitializeMeshMaterialData(ViewIfDynamicMeshCommand, PrimitiveSceneProxy, MeshBatch, StaticMeshId, false);

    const FMeshDrawCommandSortKey SortKey = CalculateMeshStaticSortKey(ToonBasePassShaders.VertexShader, ToonBasePassShaders.PixelShader);
	PassDrawRenderState.SetDepthStencilState(TStaticDepthStencilState<false, CF_DepthNearOrEqual>().GetRHI());

	FMeshPassProcessorRenderState DrawRenderState(PassDrawRenderState);
	
    BuildMeshDrawCommands(
        MeshBatch,
        BatchElementMask,
        PrimitiveSceneProxy,
        MaterialRenderProxy,
        MaterialResource,
        DrawRenderState,
        ToonBasePassShaders,
        MeshFillMode,
        MeshCullMode,
        SortKey,
        EMeshPassFeatures::Default,
        ShaderElementData
    );

    return true;
}

//------------------FRegisterPassProcessorCreateFunction---------------

void SetupToonBasePassState(FMeshPassProcessorRenderState& DrawRenderState)
{
	DrawRenderState.SetDepthStencilState(TStaticDepthStencilState<false, CF_DepthNearOrEqual>::GetRHI());
}

FMeshPassProcessor* CreateToonBasePassProcessor(ERHIFeatureLevel::Type FeatureLevel, const FScene* Scene, const FSceneView* InViewIfDynamicMeshCommand, FMeshPassDrawListContext* InDrawListContext)
{
	FMeshPassProcessorRenderState ToonPassState;
	SetupToonBasePassState(ToonPassState);
	return new FToonBasePassProcessor(Scene, InViewIfDynamicMeshCommand, ToonPassState, InDrawListContext);
}

// RegisterToonPass会将CreateToonPassProcessor函数的地址写入FPassProcessorManager的一个Table里，Table的下标是EShadingPath和EMeshPass
// 这个Table包括了所以Pass的CreatePassProcessor函数，之后引擎就可以根据EShadingPath和EMeshPass找到对应pass的CreatePassProcessor函数
FRegisterPassProcessorCreateFunction RegisterToonPass(&CreateToonBasePassProcessor, EShadingPath::Deferred, EMeshPass::ToonBasePass, EMeshPassFlags::CachedMeshCommands | EMeshPassFlags::MainView);

//------------------FRegisterPassProcessorCreateFunction---------------


DECLARE_CYCLE_STAT(TEXT("ToonBasePass"), STAT_CLP_ToonBasePass, STATGROUP_SceneRendering);

BEGIN_SHADER_PARAMETER_STRUCT(FToonBasePassParameters, )
    SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
    SHADER_PARAMETER_STRUCT_INCLUDE(FInstanceCullingDrawParams, InstanceCullingDrawParams)
    RENDER_TARGET_BINDING_SLOTS()
END_SHADER_PARAMETER_STRUCT()

FToonBasePassParameters* GetToonPassParameters(FRDGBuilder& GraphBuilder, const FViewInfo& View, FSceneTextures& SceneTextures)
{
    FToonBasePassParameters* PassParameters = GraphBuilder.AllocParameters<FToonBasePassParameters>();
    PassParameters->View = View.ViewUniformBuffer;

	if (!HasBeenProduced(SceneTextures.TBufferA))
	{
		const FSceneTexturesConfig& Config = View.GetSceneTexturesConfig();
		SceneTextures.TBufferA = CreateToonBufferTexture(GraphBuilder, Config.Extent, GFastVRamConfig.TBufferA, TEXT("TBufferA"));
		SceneTextures.TBufferB = CreateToonBufferTexture(GraphBuilder, Config.Extent, GFastVRamConfig.TBufferB, TEXT("TBufferB"));
		SceneTextures.TBufferC = CreateToonBufferTexture(GraphBuilder, Config.Extent, GFastVRamConfig.TBufferC, TEXT("TBufferC"));
	}
	// 设置RenderTarget
	PassParameters->RenderTargets[0] = FRenderTargetBinding(SceneTextures.TBufferA, ERenderTargetLoadAction::ELoad);
	PassParameters->RenderTargets[1] = FRenderTargetBinding(SceneTextures.TBufferB, ERenderTargetLoadAction::ELoad);
	PassParameters->RenderTargets[2] = FRenderTargetBinding(SceneTextures.TBufferC, ERenderTargetLoadAction::ELoad);
	PassParameters->RenderTargets.DepthStencil = FDepthStencilBinding(SceneTextures.Depth.Target, ERenderTargetLoadAction::ELoad, ERenderTargetLoadAction::ELoad, FExclusiveDepthStencil::DepthWrite_StencilWrite);

	return PassParameters;
}

// 用于清空TBuffer的函数
void ClearToonBuffer(FRDGBuilder& GraphBuilder, const FViewInfo& View, FSceneTextures& SceneTextures)
{
	if (!HasBeenProduced(SceneTextures.TBufferA) || !HasBeenProduced(SceneTextures.TBufferB) || !HasBeenProduced(SceneTextures.TBufferC))
	{
		// 如果ToonBuffer没被创建，在这里创建
		const FSceneTexturesConfig& Config = View.GetSceneTexturesConfig();
		CreateToonBuffers(GraphBuilder, SceneTextures, Config.Extent, GFastVRamConfig);
	}
	FToonBasePassParameters* PassParameters = GraphBuilder.AllocParameters<FToonBasePassParameters>();
	PassParameters->RenderTargets[0] = FRenderTargetBinding(SceneTextures.TBufferA, ERenderTargetLoadAction::ENoAction);
	PassParameters->RenderTargets[1] = FRenderTargetBinding(SceneTextures.TBufferB, ERenderTargetLoadAction::ENoAction);
	PassParameters->RenderTargets[2] = FRenderTargetBinding(SceneTextures.TBufferC, ERenderTargetLoadAction::ENoAction);
	
	GraphBuilder.AddPass(RDG_EVENT_NAME("TBufferClear"), PassParameters, ERDGPassFlags::Raster,
			[PassParameters](FRHICommandList& RHICmdList)
		{
			// If no fast-clear action was used, we need to do an MRT shader clear.
			const FRenderTargetBindingSlots& RenderTargets = PassParameters->RenderTargets;
			FLinearColor ClearColors[MaxSimultaneousRenderTargets];
			FRHITexture* Textures[MaxSimultaneousRenderTargets];
			int32 TextureIndex = 0;

			RenderTargets.Enumerate([&](const FRenderTargetBinding& RenderTarget)
			{
				FRHITexture* TextureRHI = RenderTarget.GetTexture()->GetRHI();
				ClearColors[TextureIndex] = TextureRHI->GetClearColor();
				Textures[TextureIndex] = TextureRHI;
				++TextureIndex;
			});

			DrawClearQuadMRT(RHICmdList, true, TextureIndex, ClearColors, false, 0, false, 0);
			
		});
}

// 在DeferredShadingSceneRenderer调用这个函数来渲染ToonPass
void FDeferredShadingSceneRenderer::RenderToonBasePass(FRDGBuilder& GraphBuilder, FSceneTextures& SceneTextures)
{
    RDG_EVENT_SCOPE(GraphBuilder, "ToonBasePass");
    RDG_CSV_STAT_EXCLUSIVE_SCOPE(GraphBuilder, RenderToonBasePass);

    SCOPED_NAMED_EVENT(FDeferredShadingSceneRenderer_RenderToonBasePass, FColor::Emerald);

    for(int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex)
    {
        FViewInfo& View = Views[ViewIndex];
        RDG_GPU_MASK_SCOPE(GraphBuilder, View.GPUMask);
        RDG_EVENT_SCOPE_CONDITIONAL(GraphBuilder, Views.Num() > 1, "View%d", ViewIndex);

        const bool bShouldRenderView = View.ShouldRenderView();
        if(bShouldRenderView)
        {
        	ClearToonBuffer(GraphBuilder, View, SceneTextures);
            FToonBasePassParameters* PassParameters = GetToonPassParameters(GraphBuilder, View, SceneTextures);

            View.ParallelMeshDrawCommandPasses[EMeshPass::ToonBasePass].BuildRenderingCommands(GraphBuilder, Scene->GPUScene, PassParameters->InstanceCullingDrawParams);
            GraphBuilder.AddDispatchPass(
                RDG_EVENT_NAME("ToonBasePassParallel"),
                PassParameters,
                ERDGPassFlags::Raster | ERDGPassFlags::SkipRenderPass,
                [&View, PassParameters](FRDGDispatchPassBuilder& DispatchPassBuilder)
            {
                View.ParallelMeshDrawCommandPasses[EMeshPass::ToonBasePass].Dispatch(DispatchPassBuilder, &PassParameters->InstanceCullingDrawParams);
                // FRDGParallelCommandListSet ParallelCommandListSet(InPass, RHICmdList, GET_STATID(STAT_CLP_ToonPass), View, FParallelCommandListBindings(PassParameters));
                // ParallelCommandListSet.SetHighPriority();
                // View.ParallelMeshDrawCommandPasses[EMeshPass::ToonBasePass].DispatchDraw(&ParallelCommandListSet, RHICmdList, &PassParameters->InstanceCullingDrawParams);
            });
        }
    }
}