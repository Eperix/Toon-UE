#include "ToonPassRendering.h"

#include "ScenePrivate.h"
#include "MeshPassProcessor.inl"
#include "SimpleMeshDrawCommandPass.h"
#include "StaticMeshBatch.h"
#include "DeferredShadingRenderer.h"
#include "PipelineFileCache.h"

// IMPLEMENT_MATERIAL_SHADER_TYPE接受的参数：
// FToonPassPS:我们在ToonPassRendering.h中定义的shader类
// TEXT("/Engine/Private/Toon/ToonPassShader.usf"):我们使用的shader路径
// TEXT("MainPS"):shader的入口函数名
// SF_Pixel:shader的类型，Vertex shader、Pixel shader或者compute shader
IMPLEMENT_MATERIAL_SHADER_TYPE(, FToonPassVS, TEXT("/Engine/Private/Toon/ToonShaders.usf"), TEXT("MainVS"), SF_Vertex);
IMPLEMENT_MATERIAL_SHADER_TYPE(, FToonPassPS, TEXT("/Engine/Private/Toon/ToonShaders.usf"), TEXT("MainPS"), SF_Pixel);

//--------------------------------------------Toon Buffer Texture---------------------------------------------
// Toon Buffer step 5-2
FRDGTextureDesc GetToonBufferTextureDesc(FIntPoint Extent, ETextureCreateFlags CreateFlags)
{
	//输入的参数：
	//Extent：贴图尺寸；PF_B8G8R8A8_UINT：贴图格式，表示RGBA各个通道均为8bit uint
	//FClearValueBinding::Black:清除值，表示清除贴图时将其清除为黑色
	//TexCreate_UAV：Unordered Access View，允许在着色器中进行随机读写操作
	//TexCreate_RenderTargetable：表示纹理可作为渲染目标使用
	//TexCreate_ShaderResource：表示纹理可作为着色器资源，可以在着色器中进行采样等操作
	return FRDGTextureDesc(FRDGTextureDesc::Create2D(Extent, PF_R8G8B8A8_UINT, FClearValueBinding::Black, TexCreate_UAV | TexCreate_RenderTargetable | TexCreate_ShaderResource | CreateFlags));
}
// Toon Buffer step 5-3
FRDGTextureRef CreateToonBufferTexture(FRDGBuilder& GraphBuilder, FIntPoint Extent, ETextureCreateFlags CreateFlags, const TCHAR* Name)
{	
	return GraphBuilder.CreateTexture(GetToonBufferTextureDesc(Extent, CreateFlags), Name);
}

 //------------------------------------------- Mesh Pass Processor-------------------------------------------------

FToonPassProcessor::FToonPassProcessor(
	const FScene* Scene,  
	const FSceneView* InViewIfDynamicMeshCommand,  
	const FMeshPassProcessorRenderState& InPassDrawRenderState,  
	FMeshPassDrawListContext* InDrawListContext )
:FMeshPassProcessor(EMeshPass::ToonMeshPass, Scene, Scene->GetFeatureLevel(), InViewIfDynamicMeshCommand, InDrawListContext),
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

void FToonPassProcessor::AddMeshBatch(
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
	    if (ShadingModels.HasShadingModel(MSM_ToonDefault) || ShadingModels.HasShadingModel(MSM_ToonEye) || ShadingModels.HasShadingModel(MSM_ToonSkin) || ShadingModels.HasShadingModel(MSM_ToonHair))
	    {
	    	const EBlendMode BlendMode = Material->GetBlendMode();

	    	bool bResult = true;
	    	if (BlendMode == BLEND_Opaque)
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

bool FToonPassProcessor::Process(
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

    TMeshProcessorShaders<FToonPassVS, FToonPassVS> ToonPassShader;
    {
        FMaterialShaderTypes ShaderTypes;
    	// 指定使用的shader
        ShaderTypes.AddShaderType<FToonPassVS>();
        ShaderTypes.AddShaderType<FToonPassPS>();

        const FVertexFactoryType* VertexFactoryType = VertexFactory->GetType();

        FMaterialShaders Shaders;
        if (!MaterialResource.TryGetShaders(ShaderTypes, VertexFactoryType, Shaders))
        {
            //UE_LOG(LogShaders, Warning, TEXT("Shader Not Found!"));
            return false;
        }

        Shaders.TryGetVertexShader(ToonPassShader.VertexShader);
        Shaders.TryGetPixelShader(ToonPassShader.PixelShader);
    }


    FMeshMaterialShaderElementData ShaderElementData;
    ShaderElementData.InitializeMeshMaterialData(ViewIfDynamicMeshCommand, PrimitiveSceneProxy, MeshBatch, StaticMeshId, false);

    const FMeshDrawCommandSortKey SortKey = CalculateMeshStaticSortKey(ToonPassShader.VertexShader, ToonPassShader.PixelShader);
	PassDrawRenderState.SetDepthStencilState(TStaticDepthStencilState<false, CF_DepthNearOrEqual>().GetRHI());

	FMeshPassProcessorRenderState DrawRenderState(PassDrawRenderState);
	
    BuildMeshDrawCommands(
        MeshBatch,
        BatchElementMask,
        PrimitiveSceneProxy,
        MaterialRenderProxy,
        MaterialResource,
        DrawRenderState,
        ToonPassShader,
        MeshFillMode,
        MeshCullMode,
        SortKey,
        EMeshPassFeatures::Default,
        ShaderElementData
    );

    return true;
}

//------------------FRegisterPassProcessorCreateFunction---------------

void SetupToonPassState(FMeshPassProcessorRenderState& DrawRenderState)
{
	DrawRenderState.SetDepthStencilState(TStaticDepthStencilState<false, CF_DepthNearOrEqual>::GetRHI());
}

FMeshPassProcessor* CreateToonPassProcessor(ERHIFeatureLevel::Type FeatureLevel, const FScene* Scene, const FSceneView* InViewIfDynamicMeshCommand, FMeshPassDrawListContext* InDrawListContext)
{
	FMeshPassProcessorRenderState ToonPassState;
	SetupToonPassState(ToonPassState);
	return new FToonPassProcessor(Scene, InViewIfDynamicMeshCommand, ToonPassState, InDrawListContext);
}

// RegisterToonPass会将CreateToonPassProcessor函数的地址写入FPassProcessorManager的一个Table里，Table的下标是EShadingPath和EMeshPass
// 这个Table包括了所以Pass的CreatePassProcessor函数，之后引擎就可以根据EShadingPath和EMeshPass找到对应pass的CreatePassProcessor函数
FRegisterPassProcessorCreateFunction RegisterToonPass(&CreateToonPassProcessor, EShadingPath::Deferred, EMeshPass::ToonMeshPass, EMeshPassFlags::CachedMeshCommands | EMeshPassFlags::MainView);

//------------------FRegisterPassProcessorCreateFunction---------------


DECLARE_CYCLE_STAT(TEXT("ToonPass"), STAT_CLP_ToonPass, STATGROUP_SceneRendering);

BEGIN_SHADER_PARAMETER_STRUCT(FToonMeshPassParameters, )
    SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
    SHADER_PARAMETER_STRUCT_INCLUDE(FInstanceCullingDrawParams, InstanceCullingDrawParams)
    RENDER_TARGET_BINDING_SLOTS()
END_SHADER_PARAMETER_STRUCT()

FToonMeshPassParameters* GetToonPassParameters(FRDGBuilder& GraphBuilder, const FViewInfo& View, FSceneTextures& SceneTextures)
{
    FToonMeshPassParameters* PassParameters = GraphBuilder.AllocParameters<FToonMeshPassParameters>();
    PassParameters->View = View.ViewUniformBuffer;

	if (!HasBeenProduced(SceneTextures.TBufferA))
	{
		const FSceneTexturesConfig& Config = View.GetSceneTexturesConfig();
		SceneTextures.TBufferA = CreateToonBufferTexture(GraphBuilder, Config.Extent, GFastVRamConfig.TBufferA, TEXT("TBufferA"));
		SceneTextures.TBufferB = CreateToonBufferTexture(GraphBuilder, Config.Extent, GFastVRamConfig.TBufferB, TEXT("TBufferB"));
		SceneTextures.TBufferC = CreateToonBufferTexture(GraphBuilder, Config.Extent, GFastVRamConfig.TBufferC, TEXT("TBufferC"));
	}
	// 设置RenderTarget
    PassParameters->RenderTargets[0] = FRenderTargetBinding(SceneTextures.TBufferA, ERenderTargetLoadAction::EClear);
    PassParameters->RenderTargets[1] = FRenderTargetBinding(SceneTextures.TBufferB, ERenderTargetLoadAction::EClear);
    PassParameters->RenderTargets[2] = FRenderTargetBinding(SceneTextures.TBufferC, ERenderTargetLoadAction::EClear);
    PassParameters->RenderTargets.DepthStencil = FDepthStencilBinding(SceneTextures.Depth.Target, ERenderTargetLoadAction::ELoad, ERenderTargetLoadAction::ELoad, FExclusiveDepthStencil::DepthWrite_StencilWrite);

    return PassParameters;
}

// 在DeferredShadingSceneRenderer调用这个函数来渲染ToonPass
void FDeferredShadingSceneRenderer::RenderToonPass(FRDGBuilder& GraphBuilder, FSceneTextures& SceneTextures)
{
    RDG_EVENT_SCOPE(GraphBuilder, "ToonPass");
    RDG_CSV_STAT_EXCLUSIVE_SCOPE(GraphBuilder, RenderToonPass);

    SCOPED_NAMED_EVENT(FDeferredShadingSceneRenderer_RenderToonPass, FColor::Emerald);

    for(int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex)
    {
        FViewInfo& View = Views[ViewIndex];
        RDG_GPU_MASK_SCOPE(GraphBuilder, View.GPUMask);
        RDG_EVENT_SCOPE_CONDITIONAL(GraphBuilder, Views.Num() > 1, "View%d", ViewIndex);

        const bool bShouldRenderView = View.ShouldRenderView();
        if(bShouldRenderView)
        {
            FToonMeshPassParameters* PassParameters = GetToonPassParameters(GraphBuilder, View, SceneTextures);

            View.ParallelMeshDrawCommandPasses[EMeshPass::ToonMeshPass].BuildRenderingCommands(GraphBuilder, Scene->GPUScene, PassParameters->InstanceCullingDrawParams);
            GraphBuilder.AddDispatchPass(
                RDG_EVENT_NAME("ToonPass"),
                PassParameters,
                ERDGPassFlags::Raster | ERDGPassFlags::SkipRenderPass,
                [&View, PassParameters](FRDGDispatchPassBuilder& DispatchPassBuilder)
            {
                View.ParallelMeshDrawCommandPasses[EMeshPass::ToonMeshPass].Dispatch(DispatchPassBuilder, &PassParameters->InstanceCullingDrawParams);
                // FRDGParallelCommandListSet ParallelCommandListSet(InPass, RHICmdList, GET_STATID(STAT_CLP_ToonPass), View, FParallelCommandListBindings(PassParameters));
                // ParallelCommandListSet.SetHighPriority();
                // View.ParallelMeshDrawCommandPasses[EMeshPass::ToonMeshPass].DispatchDraw(&ParallelCommandListSet, RHICmdList, &PassParameters->InstanceCullingDrawParams);
            });
        }
    }
}