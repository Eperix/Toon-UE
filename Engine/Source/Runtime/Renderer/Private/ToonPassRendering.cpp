// #include "ToonPassRendering.h"
//
// #include "BasePassRendering.h"
// #include "DeferredShadingRenderer.h"
// #include "ScenePrivate.h"
// #include "MeshPassProcessor.inl"
// #include "Engine/RendererSettings.h"
// #include "Materials/MaterialRenderProxy.h"
//
// BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
// 	SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
// 	SHADER_PARAMETER_STRUCT_INCLUDE(FInstanceCullingDrawParams, InstanceCullingDrawParams)
// 	RENDER_TARGET_BINDING_SLOTS()
// END_SHADER_PARAMETER_STRUCT()
//
// // Shader Start
// class FToonPassVS : public FMeshMaterialShader
// {
// 	// 声明Shader，实际上是初始化各种指针以及类型“MeshMaterial”
// 	DECLARE_SHADER_TYPE(FToonPassVS, MeshMaterial);
// 	SHADER_USE_PARAMETER_STRUCT(FToonPassVS, FMeshMaterialShader);
// public:
// 	// 修改Shader环境
// 	static void ModifyCompilationEnvironment(const FMaterialShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
// 	{}
//
// 	// 根据平台/数据决定是否应该编译
// 	static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters)
// 	{
// 		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5) &&
// 			(Parameters.VertexFactoryType->GetFName() == FName(TEXT("FLocalVertexFactory")) || 
// 				Parameters.VertexFactoryType->GetFName() == FName(TEXT("TGPUSkinVertexFactoryDefault")));
// 	}
// };
//
// class FToonPassPS : public FMeshMaterialShader
// {
// 	DECLARE_SHADER_TYPE(FToonPassPS, MeshMaterial);
// 	SHADER_USE_PARAMETER_STRUCT(FToonPassPS, FMeshMaterialShader);
// 	
// 	static void ModifyCompilationEnvironment(const FMaterialShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
// 	{}
//
// 	static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters)
// 	{
// 		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5) &&
// 			(Parameters.VertexFactoryType->GetFName() == FName(TEXT("FLocalVertexFactory")) || 
// 				Parameters.VertexFactoryType->GetFName() == FName(TEXT("TGPUSkinVertexFactoryDefault")));
// 	}
// };
// IMPLEMENT_MATERIAL_SHADER_TYPE(, FToonPassVS, TEXT("/Engine/Private/Toon/ToonShaders.usf"), TEXT("MainVS"), SF_Vertex);
// IMPLEMENT_MATERIAL_SHADER_TYPE(, FToonPassPS, TEXT("/Engine/Private/Toon/ToonShaders.usf"), TEXT("MainPS"), SF_Pixel);
//
// //Shader End
//
// // Processor Start
// FToonPassProcessor::FToonPassProcessor(const FScene* Scene,
// 	ERHIFeatureLevel::Type InFeatureLevel,
// 	const FSceneView* InViewIfDynamicMeshCommand,
// 	const FMeshPassProcessorRenderState& InPassDrawRenderState,
// 	FMeshPassDrawListContext* InDrawListContext)
// :FMeshPassProcessor(EMeshPass::ToonMeshPass, Scene, InFeatureLevel, InViewIfDynamicMeshCommand, InDrawListContext),
// PassDrawRenderState(InPassDrawRenderState)
// {
// 	// 设置默认的BlendState、DepthStencil等
// 	if (PassDrawRenderState.GetDepthStencilState() == nullptr)
// 	{
// 		PassDrawRenderState.SetDepthStencilState(TStaticDepthStencilState<false, CF_DepthNearOrEqual>().GetRHI());
// 	}
// 	if (PassDrawRenderState.GetBlendState() == nullptr)
// 	{
// 		PassDrawRenderState.SetBlendState(TStaticBlendState<>().GetRHI());
// 	}
// 	
// }
//
// void FToonPassProcessor::AddMeshBatch(const FMeshBatch& MeshBatch, uint64 BatchElementMask, const FPrimitiveSceneProxy* PrimitiveSceneProxy,
// 	int32 StaticMeshId)
// {
// 	const FMaterialRenderProxy* MaterialRenderProxy = MeshBatch.MaterialRenderProxy;
//
// 	const FMaterial* Material = MaterialRenderProxy->GetMaterialNoFallback(FeatureLevel);
//
// 	// 只对特定的着色模型 && 不透明物体进行该Pass
// 	if (Material && Material->GetRenderingThreadShaderMap())
// 	{
// 		const FMaterialShadingModelField ShadingModels = Material->GetShadingModels();
// 		if (ShadingModels.HasAnyShadingModel({MSM_ToonDefault, MSM_ToonHair, MSM_ToonSkin}))
// 		{
// 			const EBlendMode BlendMode = Material->GetBlendMode();
// 			bool bResult = true;
// 			if (BlendMode == BLEND_Opaque)
// 			{
// 				// 指定特定的规则
// 				Process(MeshBatch, BatchElementMask, StaticMeshId, PrimitiveSceneProxy,
// 					*MaterialRenderProxy, *Material, FM_Solid, CM_CW);
// 			}
// 		}
// 	}
// }
//
// bool FToonPassProcessor::Process(const FMeshBatch& MeshBatch,
// 	uint64 BatchElementMask,
// 	int32 StaticMeshId,
// 	const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy,
// 	const FMaterialRenderProxy& RESTRICT MaterialRenderProxy,
// 	const FMaterial& RESTRICT MaterialResource,
// 	ERasterizerFillMode MeshFillMode,
// 	ERasterizerCullMode MeshCullMode)
// {
// 	
// 	const FVertexFactory* VertexFactory = MeshBatch.VertexFactory;
//
// 	// 尝试从材质中获取特定的Shader，获取不到时Shaders会被设置为空从而不进行Pass
// 	TMeshProcessorShaders<FToonPassVS, FToonPassPS> ToonPassShader;
// 	{
// 		FMaterialShaderTypes ShaderTypes;
// 		ShaderTypes.AddShaderType<FToonPassVS>();
// 		ShaderTypes.AddShaderType<FToonPassPS>();
//
// 		const FVertexFactoryType* VertexFactoryType = VertexFactory->GetType();
//
// 		FMaterialShaders Shaders;
// 		if (!MaterialResource.TryGetShaders(ShaderTypes, VertexFactoryType, Shaders))
// 		{
// 			return false;
// 		}
// 		
// 		Shaders.TryGetVertexShader(ToonPassShader.VertexShader);
// 		Shaders.TryGetPixelShader(ToonPassShader.PixelShader);
// 	}
//
// 	FMeshMaterialShaderElementData ShaderElementData;
// 	ShaderElementData.InitializeMeshMaterialData(ViewIfDynamicMeshCommand, PrimitiveSceneProxy, MeshBatch, StaticMeshId, false);
//
// 	const FMeshDrawCommandSortKey SortKey = CalculateMeshStaticSortKey(ToonPassShader.VertexShader, ToonPassShader.PixelShader);
//
// 	FMeshPassProcessorRenderState DrawRenderState(PassDrawRenderState);
// 	DrawRenderState.SetDepthStencilState(TStaticDepthStencilState<false, CF_DepthNearOrEqual>().GetRHI());
// 	
// 	// 获得所有资源和信息后BuildCommands
// 	BuildMeshDrawCommands(
// 		MeshBatch,
// 		BatchElementMask,
// 		PrimitiveSceneProxy,
// 		MaterialRenderProxy,
// 		MaterialResource,
// 		DrawRenderState,
// 		ToonPassShader,
// 		MeshFillMode,
// 		MeshCullMode,
// 		SortKey,
// 		EMeshPassFeatures::Default,
// 		ShaderElementData);
// 	
// 	return true;
// }
//
// FRDGTextureDesc CreateToonTextureDesc(FIntPoint Extent,const ETextureCreateFlags& Flags)
// {
// 	return FRDGTextureDesc(FRDGTextureDesc::Create2D(Extent, PF_B8G8R8A8, FClearValueBinding::Black, TexCreate_UAV | TexCreate_RenderTargetable | TexCreate_ShaderResource | Flags));
// }
//
// FRDGTextureRef CreateToonTexture(FRDGBuilder& GraphBuilder, FIntPoint Extent, uint8 Index)
// {
// 	switch (Index)
// 	{
// 		case 0: return GraphBuilder.CreateTexture(CreateToonTextureDesc(Extent, GFastVRamConfig.ToonTextureA), TEXT("ToonTextureA"));
// 		case 1: return GraphBuilder.CreateTexture(CreateToonTextureDesc(Extent, GFastVRamConfig.ToonTextureB), TEXT("ToonTextureB"));
// 		case 2: return GraphBuilder.CreateTexture(CreateToonTextureDesc(Extent, GFastVRamConfig.ToonTextureC), TEXT("ToonTextureC"));
// 		default:return GraphBuilder.CreateTexture(CreateToonTextureDesc(Extent, GFastVRamConfig.ToonTextureA), TEXT("ToonTextureA"));
// 	}
// }
//
// void SetupToonPassState(FMeshPassProcessorRenderState& ToonPassState)
// {
// 	ToonPassState.SetDepthStencilState(TStaticDepthStencilState<false, CF_DepthNearOrEqual>().GetRHI());
// }
//
// FMeshPassProcessor* CreateToonPassProcessor(ERHIFeatureLevel::Type FeatureLevel, const FScene* Scene, const FSceneView* InViewIfDynamicMeshCommand, FMeshPassDrawListContext* InDrawListContext)
// {
// 	FMeshPassProcessorRenderState ToonPassState;
// 	SetupToonPassState(ToonPassState);
// 	return new FToonPassProcessor(Scene, FeatureLevel, InViewIfDynamicMeshCommand, ToonPassState, InDrawListContext);
// }
// // RegisterToonPass会将CreateToonPassProcessor函数的地址写入FPassProcessorManager的一个Table里，Table的下标是EShadingPath和EMeshPass
// // 这个Table包括了所以Pass的CreatePassProcessor函数，之后引擎就可以根据EShadingPath和EMeshPass找到对应pass的CreatePassProcessor函数
// FRegisterPassProcessorCreateFunction RegisterToonPass(&CreateToonPassProcessor, EShadingPath::Deferred, EMeshPass::ToonMeshPass, EMeshPassFlags::CachedMeshCommands | EMeshPassFlags::MainView);
//
// // Processor End
//
// DECLARE_CYCLE_STAT(TEXT("ToonMeshPass"), STAT_CLP_ToonPass, STATGROUP_SceneRendering);
//
//
// FParameters* GetToonPassParameters(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FSceneTextures& SceneTextures)
// {
// 	FParameters* ToonPassParameters = GraphBuilder.AllocParameters<FParameters>();
// 	ToonPassParameters->View = View.ViewUniformBuffer;
// 	
// 	ToonPassParameters->RenderTargets[0] = FRenderTargetBinding(SceneTextures.ToonTextureA, ERenderTargetLoadAction::EClear);
// 	ToonPassParameters->RenderTargets[1] = FRenderTargetBinding(SceneTextures.ToonTextureB, ERenderTargetLoadAction::EClear);
// 	ToonPassParameters->RenderTargets[2] = FRenderTargetBinding(SceneTextures.ToonTextureC, ERenderTargetLoadAction::EClear);
// 	ToonPassParameters->RenderTargets.DepthStencil = FDepthStencilBinding(SceneTextures.Depth.Target, ERenderTargetLoadAction::ELoad, ERenderTargetLoadAction::ELoad, FExclusiveDepthStencil::DepthWrite_StencilWrite);
// 	
// 	return ToonPassParameters;
// }
//
// // TRefCountPtr<IPooledRenderTarget> ExtractedToonA;
// // TRefCountPtr<IPooledRenderTarget> ExtractedToonB;
// // TRefCountPtr<IPooledRenderTarget> ExtractedToonC;
//
// void FDeferredShadingSceneRenderer::RenderToonPass(FRDGBuilder& GraphBuilder, const FSceneTextures& SceneTextures)
// {
// 	RDG_EVENT_SCOPE(GraphBuilder, "ToonMeshPass");
// 	RDG_CSV_STAT_EXCLUSIVE_SCOPE(GraphBuilder, RenderToonPass);
//
// 	SCOPED_NAMED_EVENT(FDeferredShadingSceneRenderer_RenderToonPass, FColor::Emerald);
// 	
// 	for(int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex)
// 	{
// 		FViewInfo& View = Views[ViewIndex];
// 		RDG_GPU_MASK_SCOPE(GraphBuilder, View.GPUMask);
// 		RDG_EVENT_SCOPE_CONDITIONAL(GraphBuilder, Views.Num() > 1, "View%d", ViewIndex);
//
// 		const bool bShouldRenderView = View.ShouldRenderView();
// 		if(bShouldRenderView)
// 		{
// 			FParameters* PassParameters = GetToonPassParameters(GraphBuilder, View, SceneTextures);
// 			
// 			View.ParallelMeshDrawCommandPasses[EMeshPass::ToonMeshPass].BuildRenderingCommands(GraphBuilder, Scene->GPUScene, PassParameters->InstanceCullingDrawParams);
//
// 			GraphBuilder.AddPass(
// 				RDG_EVENT_NAME("ToonMeshPass"),
// 				PassParameters,
// 				ERDGPassFlags::Raster | ERDGPassFlags::SkipRenderPass,
// 				[this, &View, PassParameters](FRHICommandListImmediate& RHICmdList)
// 			{
// 				View.ParallelMeshDrawCommandPasses[EMeshPass::ToonMeshPass].Draw(RHICmdList, &PassParameters->InstanceCullingDrawParams);
// 			});
// 		}
// 	}
// }