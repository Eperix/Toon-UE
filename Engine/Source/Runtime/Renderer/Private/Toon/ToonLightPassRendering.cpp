#include "ToonLightPassRendering.h"

#include "ScenePrivate.h"
#include "MeshPassProcessor.inl"
#include "SimpleMeshDrawCommandPass.h"
#include "StaticMeshBatch.h"
#include "DeferredShadingRenderer.h"
#include "LightSceneProxy.h"
#include "SkyAtmosphereRendering.h"

IMPLEMENT_MATERIAL_SHADER_TYPE(, FToonLightPassVS, TEXT("/Engine/Private/Toon/ToonMeshPassVS.usf"), TEXT("MainVS"), SF_Vertex);
IMPLEMENT_MATERIAL_SHADER_TYPE(, FToonLightPassPS, TEXT("/Engine/Private/Toon/ToonLightPS.usf"), TEXT("MainPS"), SF_Pixel);
IMPLEMENT_SHADERPIPELINE_TYPE_VSPS(ToonLightPipeline, FToonLightPassVS, FToonLightPassPS, true);


FRDGTextureDesc GetToonShadowTextureDesc(FIntPoint Extent, ETextureCreateFlags CreateFlags)
{
	//输入的参数：
	//Extent：贴图尺寸；PF_B8G8R8A8_UINT：贴图格式，表示RGBA各个通道均为8bit uint
	//FClearValueBinding::Black:清除值，表示清除贴图时将其清除为黑色
	//TexCreate_UAV：Unordered Access View，允许在着色器中进行随机读写操作
	//TexCreate_RenderTargetable：表示纹理可作为渲染目标使用
	//TexCreate_ShaderResource：表示纹理可作为着色器资源，可以在着色器中进行采样等操作
	return FRDGTextureDesc(FRDGTextureDesc::Create2D(Extent, PF_R8G8B8A8, FClearValueBinding::Black, TexCreate_UAV | TexCreate_RenderTargetable | TexCreate_ShaderResource | CreateFlags));
}
// Toon Buffer step 5-3
FRDGTextureRef GetToonShadowTexture(FRDGBuilder& GraphBuilder, FIntPoint Extent, ETextureCreateFlags CreateFlags, const TCHAR* Name)
{	
	return GraphBuilder.CreateTexture(GetToonShadowTextureDesc(Extent, CreateFlags), Name);
}
//------------------------------------------- Mesh Pass Processor-------------------------------------------------

bool GetToonLightShader(
	const FMaterial& Material,
	const FVertexFactoryType* VertexFactoryType,
	ERHIFeatureLevel::Type FeatureLevel,
	TShaderRef<FToonLightPassVS>& VertexShader,
	TShaderRef<FToonLightPassPS>& PixelShader)
{
	FMaterialShaderTypes ShaderTypes;
	ShaderTypes.PipelineType = &ToonLightPipeline;
	ShaderTypes.AddShaderType<FToonLightPassVS>();
	ShaderTypes.AddShaderType<FToonLightPassPS>();

	FMaterialShaders Shaders;
	if(!Material.TryGetShaders(ShaderTypes , VertexFactoryType , Shaders))
	{
		return false;
	}

	Shaders.TryGetVertexShader(VertexShader);
	Shaders.TryGetPixelShader(PixelShader);
	
	check(VertexShader.IsValid() && PixelShader.IsValid());

	return true;
}

bool ShouldDrawToonLightPass(const FMaterial* Material)
{
	const FMaterialShadingModelField ShadingModels = Material->GetShadingModels();
	const EBlendMode BlendMode = Material->GetBlendMode();
	//const bool bIsNotTranslucent = BlendMode == BLEND_Opaque || BlendMode == BLEND_Masked;
	return ShadingModels.HasShadingModel(MSM_ToonDefault);
}

FToonMainLightMeshProcessor::FToonMainLightMeshProcessor(
    const FScene* Scene,
    const FSceneView* InViewIfDynamicMeshCommand,
    const FMeshPassProcessorRenderState& InPassDrawRenderState,
    FMeshPassDrawListContext* InDrawListContext)
:FMeshPassProcessor(EMeshPass::ToonLightPass, Scene, Scene->GetFeatureLevel(), InViewIfDynamicMeshCommand, InDrawListContext),
PassDrawRenderState(InPassDrawRenderState)
{
}

void FToonMainLightMeshProcessor::CollectPSOInitializers(const FSceneTexturesConfig& SceneTexturesConfig,
	const FMaterial& Material, const FPSOPrecacheVertexFactoryData& VertexFactoryData,
	const FPSOPrecacheParams& PreCacheParams, TArray<FPSOPrecacheData>& PSOInitializers)
{
	// Early out If Not Unlit
	if(!Material.GetShadingModels().IsUnlit())
	{
		return;
	}

	//Only Do deferred Path for now
	if(FScene::GetShadingPath(FeatureLevel) != EShadingPath::Deferred)
	{
		return;
	}

	const FMeshDrawingPolicyOverrideSettings OverrideSettings = ComputeMeshOverrideSettings(PreCacheParams);
	const ERasterizerFillMode MeshFillMode = ComputeMeshFillMode(Material, OverrideSettings);
	const ERasterizerCullMode MeshCullMode = ComputeMeshCullMode(Material, OverrideSettings);
	
	TMeshProcessorShaders<FToonLightPassVS, FToonLightPassPS> PassShaders;

	if(!GetToonLightShader(
		Material ,
		VertexFactoryData.VertexFactoryType ,
		FeatureLevel ,
		PassShaders.VertexShader ,
		PassShaders.PixelShader))
	{
		return;
	}

	FGraphicsPipelineRenderTargetsInfo RenderTargetsInfo;
	AddGraphicsPipelineStateInitializer(
		VertexFactoryData,
		Material,
		PassDrawRenderState,
		RenderTargetsInfo,
		PassShaders,
		MeshFillMode,
		MeshCullMode,
		(EPrimitiveType)PreCacheParams.PrimitiveType,
		EMeshPassFeatures::Default,
		true,
		PSOInitializers);
}

void FToonMainLightMeshProcessor::AddMeshBatch(
    const FMeshBatch& MeshBatch,
    uint64 BatchElementMask,
    const FPrimitiveSceneProxy* PrimitiveSceneProxy,
    int32 StaticMeshId)
{
    const FMaterialRenderProxy* MaterialRenderProxy = MeshBatch.MaterialRenderProxy;

	while (MaterialRenderProxy)
	{
		const FMaterial* Material = MaterialRenderProxy->GetMaterialNoFallback(FeatureLevel);
		if(Material && ShouldDrawToonLightPass(Material))
		{
			if(TryAddMeshBatch(MeshBatch , BatchElementMask , PrimitiveSceneProxy , StaticMeshId ,*MaterialRenderProxy ,*Material))
			{
				break;
			}
		}

		MaterialRenderProxy = MaterialRenderProxy->GetFallback(FeatureLevel);
	}
}

bool FToonMainLightMeshProcessor::TryAddMeshBatch(const FMeshBatch& MeshBatch, uint64 BatchElementMask,
	const FPrimitiveSceneProxy* PrimitiveSceneProxy, int32 StaticMeshId,
	const FMaterialRenderProxy& MaterialRenderProxy, const FMaterial& Material)
{
	//mesh Material blend mode
	const EBlendMode BlendMode = Material.GetBlendMode();
	const FMeshDrawingPolicyOverrideSettings OverrideSettings = ComputeMeshOverrideSettings(MeshBatch);
	const ERasterizerFillMode MeshFullMode = ComputeMeshFillMode(Material , OverrideSettings);
	const ERasterizerCullMode MeshCullMode = ComputeMeshCullMode(Material , OverrideSettings);
	
	return Process(MeshBatch, BatchElementMask ,StaticMeshId ,PrimitiveSceneProxy,MaterialRenderProxy , Material , MeshFullMode , MeshCullMode);
}

bool FToonMainLightMeshProcessor::Process(
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

    TMeshProcessorShaders<FToonLightPassVS, FToonLightPassPS> ToonPassShader;
	
	if (!GetToonLightShader(
		MaterialResource,
		VertexFactory->GetType(),
		FeatureLevel,
		ToonPassShader.VertexShader,
		ToonPassShader.PixelShader))
	{
		return false;
	}
	
    FMeshMaterialShaderElementData ShaderElementData;
    ShaderElementData.InitializeMeshMaterialData(ViewIfDynamicMeshCommand, PrimitiveSceneProxy, MeshBatch, StaticMeshId, true);
    //ShaderElementData.InitializeMeshMaterialData(ViewIfDynamicMeshCommand, PrimitiveSceneProxy, MeshBatch, StaticMeshId, false);

    const FMeshDrawCommandSortKey SortKey = CalculateMeshStaticSortKey(ToonPassShader.VertexShader, ToonPassShader.PixelShader);

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

void SetupToonLightPassState(FMeshPassProcessorRenderState& DrawRenderState)
{
	DrawRenderState.SetBlendState(TStaticBlendState<CW_RGBA, BO_Add, BF_One, BF_One, BO_Add, BF_One, BF_One>::GetRHI());
	DrawRenderState.SetDepthStencilState(TStaticDepthStencilState<false, CF_DepthNearOrEqual>::GetRHI());
}

FMeshPassProcessor* CreateToonLightPassProcessor(ERHIFeatureLevel::Type FeatureLevel, const FScene* Scene, const FSceneView* InViewIfDynamicMeshCommand, FMeshPassDrawListContext* InDrawListContext)
{
	FMeshPassProcessorRenderState ToonPassState;
	SetupToonLightPassState(ToonPassState);
	return new FToonMainLightMeshProcessor(Scene, InViewIfDynamicMeshCommand, ToonPassState, InDrawListContext);
}

// RegisterToonPass会将CreateToonPassProcessor函数的地址写入FPassProcessorManager的一个Table里，Table的下标是EShadingPath和EMeshPass
// 这个Table包括了所以Pass的CreatePassProcessor函数，之后引擎就可以根据EShadingPath和EMeshPass找到对应pass的CreatePassProcessor函数
FRegisterPassProcessorCreateFunction RegisterToonLightPass(&CreateToonLightPassProcessor, EShadingPath::Deferred, EMeshPass::ToonLightPass, EMeshPassFlags::CachedMeshCommands | EMeshPassFlags::MainView);

//------------------FRegisterPassProcessorCreateFunction---------------

DECLARE_CYCLE_STAT(TEXT("ToonLightPass"), STAT_CLP_ToonLightPass, STATGROUP_SceneRendering);

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FToonLightUniformParameters, )
SHADER_PARAMETER_RDG_TEXTURE(Texture2D, ToonShadowTexture)
SHADER_PARAMETER_SAMPLER(SamplerState, ToonShadowTextureSampler)
SHADER_PARAMETER_STRUCT_INCLUDE(FLightShaderParameters, LightParameters)
END_GLOBAL_SHADER_PARAMETER_STRUCT()

IMPLEMENT_STATIC_UNIFORM_BUFFER_SLOT(ToonLight);
IMPLEMENT_STATIC_UNIFORM_BUFFER_STRUCT(FToonLightUniformParameters, "ToonLight", ToonLight);

BEGIN_SHADER_PARAMETER_STRUCT(FToonLightPassParameters, )
	SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
	SHADER_PARAMETER_STRUCT_INCLUDE(FInstanceCullingDrawParams, InstanceCullingDrawParams)
	SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FToonLightUniformParameters, ToonLight)
	RENDER_TARGET_BINDING_SLOTS()
END_SHADER_PARAMETER_STRUCT()

FToonLightPassParameters* GetToonLightPassParameters(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FScene* Scene, FSceneTextures& SceneTextures)
{
	FToonLightPassParameters* PassParameters = GraphBuilder.AllocParameters<FToonLightPassParameters>();
	// Set ToonLight Data
	FToonLightUniformParameters& ToonUniformParameters = *GraphBuilder.AllocParameters<FToonLightUniformParameters>();
	{
		const FRDGTextureRef WhiteDummy = GSystemTextures.GetWhiteDummy(GraphBuilder);
		ToonUniformParameters.ToonShadowTexture = WhiteDummy;
		ToonUniformParameters.ToonShadowTextureSampler = TStaticSamplerState<SF_Point, AM_Wrap, AM_Wrap, AM_Wrap>::GetRHI();
		ToonUniformParameters.LightParameters.Color = FVector3f::Zero();
		ToonUniformParameters.LightParameters.Direction = FVector3f::Zero();
		ToonUniformParameters.LightParameters.SourceRadius =0;
		ToonUniformParameters.LightParameters.SoftSourceRadius = 0;

		const FLightSceneInfo* MainLight = Scene->AtmosphereLights[0];
		if (MainLight)
		{
			ToonUniformParameters.ToonShadowTexture = SceneTextures.ToonShadow;
			FLightRenderParameters LightRenderParameters;
			MainLight->Proxy->GetLightShaderParameters(LightRenderParameters);
			LightRenderParameters.MakeShaderParameters(View.ViewMatrices, View.GetLastEyeAdaptationExposure(), ToonUniformParameters.LightParameters);
		}
	}
	
	PassParameters->View = View.ViewUniformBuffer;
	PassParameters->ToonLight = GraphBuilder.CreateUniformBuffer(&ToonUniformParameters);
	PassParameters->RenderTargets[0] = FRenderTargetBinding(SceneTextures.Color.Target, ERenderTargetLoadAction::ELoad);
	PassParameters->RenderTargets.DepthStencil =
		FDepthStencilBinding(SceneTextures.Depth.Target, ERenderTargetLoadAction::ELoad,
			ERenderTargetLoadAction::ELoad, FExclusiveDepthStencil::DepthWrite_StencilWrite);
	
	return PassParameters;
}

// 在DeferredShadingSceneRenderer调用这个函数来渲染ToonLightPass
void FDeferredShadingSceneRenderer::RenderToonLightPass(FRDGBuilder& GraphBuilder, FSceneTextures& SceneTextures)
{
    RDG_EVENT_SCOPE(GraphBuilder, "ToonLightPass");
    RDG_CSV_STAT_EXCLUSIVE_SCOPE(GraphBuilder, RenderToonLightPass);
	
    SCOPED_NAMED_EVENT(FDeferredShadingSceneRenderer_RenderToonLightPass, FColor::Emerald);
	
    for(int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex)
    {
        FViewInfo& View = Views[ViewIndex];
        RDG_GPU_MASK_SCOPE(GraphBuilder, View.GPUMask);
        RDG_EVENT_SCOPE_CONDITIONAL(GraphBuilder, Views.Num() > 1, "View%d", ViewIndex);

        const bool bShouldRenderView = View.ShouldRenderView();
        if(bShouldRenderView)
        {
        	FParallelMeshDrawCommandPass& ParallelMeshPass = View.ParallelMeshDrawCommandPasses[EMeshPass::ToonLightPass];
        	if (!ParallelMeshPass.HasAnyDraw())
        	{
        		continue;
        	}

        	View.BeginRenderView();
        	
            FToonLightPassParameters* PassParameters = GetToonLightPassParameters(GraphBuilder, View, Scene,SceneTextures);
            ParallelMeshPass.BuildRenderingCommands(GraphBuilder, Scene->GPUScene, PassParameters->InstanceCullingDrawParams);

            GraphBuilder.AddDispatchPass(
                RDG_EVENT_NAME("ToonMainLight"),
                PassParameters,
                ERDGPassFlags::Raster | ERDGPassFlags::SkipRenderPass,
                [this, &View, &ParallelMeshPass, PassParameters](FRDGDispatchPassBuilder& DispatchPassBuilder)
            {
                ParallelMeshPass.Dispatch(DispatchPassBuilder, &PassParameters->InstanceCullingDrawParams);
            });
        }
    }
}

REGISTER_MESHPASSPROCESSOR_AND_PSOCOLLECTOR(StylizedData, CreateToonLightPassProcessor , EShadingPath::Deferred, EMeshPass::ToonLightPass, EMeshPassFlags::CachedMeshCommands | EMeshPassFlags::MainView);