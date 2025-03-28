#include "ToonOutlinePassRendering.h"

#include "DeferredShadingRenderer.h"
#include "ScenePrivate.h"
#include "MeshPassProcessor.inl"
#include "Materials/MaterialRenderProxy.h"

class FToonOutlineVS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FToonOutlineVS, MeshMaterial);
public:
	FToonOutlineVS() = default;
	
	// if we has params bind them here
	FToonOutlineVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FMeshMaterialShader(Initializer)
	{
		// OutlineWidth.Bind(Initializer.ParameterMap, TEXT("OutlineWidth"));
		//BindSceneTextureUniformBufferDependentOnShadingPath(Initializer, PassUniformBuffer, PassUniformBuffer);  
	}

	// Set Define in Shader. 
	static void ModifyCompilationEnvironment(const FMaterialShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)  
	{         
		// OutEnvironment.SetDefine(TEXT("Define"), Value);  
	}  

	//return VertexFactoryType->SupportsPositionOnly() && Material->IsSpecialEngineMaterial();
	static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters)  
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5) &&  
		   (Parameters.VertexFactoryType->GetFName() == FName(TEXT("FLocalVertexFactory")) ||   
			  Parameters.VertexFactoryType->GetFName() == FName(TEXT("TGPUSkinVertexFactoryDefault")));  
	}  
	// You can call this function to bind every mesh personality data
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

		// Get Data from Material  
		// const float OutlineWidthFromMat = Material.GetOutlineWidth();  
		// ShaderBindings.Add(OutlineWidth, OutlineWidthFromMat);  
	}  
	/** The parameter to use for setting the Mesh OutLine Scale. */ 
	// LAYOUT_FIELD(FShaderParameter, OutlineWidth)
};

class FToonOutlinePS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FToonOutlinePS, MeshMaterial);
public:
	FToonOutlinePS() = default;  
    FToonOutlinePS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)  
       : FMeshMaterialShader(Initializer)  
    {  
       // if we has color bind it here  
       // OutlineColor.Bind(Initializer.ParameterMap, TEXT("OutlineColor"));  
    }  
    static void ModifyCompilationEnvironment(const FMaterialShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)  
    {       // Set Define in Shader.   
       //OutEnvironment.SetDefine(TEXT("Define"), Value);  
    }  

    // FShader interface.  
    static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters)  
    {       //return VertexFactoryType->SupportsPositionOnly() && Material->IsSpecialEngineMaterial();  
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
       FMeshMaterialShader::GetShaderBindings(Scene, FeatureLevel, PrimitiveSceneProxy, MaterialRenderProxy, Material,  ShaderElementData, ShaderBindings);  

       // Get ToonOutLine Data from Material  
       // const FLinearColor OutlineColorFromMat = Material.GetOutlineColor();  
       // FVector3f Color(OutlineColorFromMat.R, OutlineColorFromMat.G, OutlineColorFromMat.G);  
       //FVector3f Color(1.0, 0.0, 0.0);  
       // Bind to Shader       
       // ShaderBindings.Add(OutlineColor, Color);  
    }    
    /** The parameter to use for setting the Mesh OutLine Color. */  
    // LAYOUT_FIELD(FShaderParameter, OutlineColor); 
};

IMPLEMENT_MATERIAL_SHADER_TYPE(, FToonOutlineVS, TEXT("/Engine/Private/Toon/ToonOutline.usf"), TEXT("MainVS"), SF_Vertex);
IMPLEMENT_MATERIAL_SHADER_TYPE(, FToonOutlinePS, TEXT("/Engine/Private/Toon/ToonOutline.usf"), TEXT("MainPS"), SF_Pixel);

FToonOutlinePassProcessor::FToonOutlinePassProcessor(  
	const FScene* Scene,   
	const FSceneView* InViewIfDynamicMeshCommand,   
	const FMeshPassProcessorRenderState& InPassDrawRenderState,   
	FMeshPassDrawListContext* InDrawListContext)  
	:FMeshPassProcessor(EMeshPass::ToonOutlinePass, Scene, Scene->GetFeatureLevel(), InViewIfDynamicMeshCommand, InDrawListContext),  
	PassDrawRenderState(InPassDrawRenderState)  
{  
	if (PassDrawRenderState.GetDepthStencilState() == nullptr)  
	{       
		PassDrawRenderState.SetDepthStencilState(TStaticDepthStencilState<false, CF_DepthNearOrEqual>().GetRHI());  
	}    
	if (PassDrawRenderState.GetBlendState() == nullptr)  
	{       
		PassDrawRenderState.SetBlendState(TStaticBlendState<>().GetRHI());  
	}
}

void FToonOutlinePassProcessor::AddMeshBatch(  
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
	   // Only Toon shading model and enable render toon outline can render this pass  
	   if (ShadingModels.HasAnyShadingModel({MSM_ToonDefault, MSM_ToonHair}))
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
					CM_CCW); // Cull back  
			}  
		}    
	}
}

bool FToonOutlinePassProcessor::Process(  
    const FMeshBatch& MeshBatch,  
    uint64 BatchElementMask,  
    int32 StaticMeshId,  
    const FPrimitiveSceneProxy* PrimitiveSceneProxy,  
    const FMaterialRenderProxy& MaterialRenderProxy,  
    const FMaterial& MaterialResource,  
    ERasterizerFillMode MeshFillMode,  
    ERasterizerCullMode MeshCullMode)  
{  
    const FVertexFactory* VertexFactory = MeshBatch.VertexFactory;  

    TMeshProcessorShaders<FToonOutlineVS, FToonOutlinePS> ToonOutlinePassShader;  
    {       
        FMaterialShaderTypes ShaderTypes;  
        // 指定使用的shader  
        ShaderTypes.AddShaderType<FToonOutlineVS>();  
        ShaderTypes.AddShaderType<FToonOutlinePS>();  

        const FVertexFactoryType* VertexFactoryType = VertexFactory->GetType();  

        FMaterialShaders Shaders;  
        if (!MaterialResource.TryGetShaders(ShaderTypes, VertexFactoryType, Shaders))  
        {          
            // UE_LOG(LogShaders, Warning, TEXT("Shader Not Found in Outline Shaders"));  
            return false;  
        }  
       Shaders.TryGetVertexShader(ToonOutlinePassShader.VertexShader);  
       Shaders.TryGetPixelShader(ToonOutlinePassShader.PixelShader);  
    }  

    FMeshMaterialShaderElementData ShaderElementData;  
    ShaderElementData.InitializeMeshMaterialData(ViewIfDynamicMeshCommand, PrimitiveSceneProxy, MeshBatch, StaticMeshId, false);  

    const FMeshDrawCommandSortKey SortKey = CalculateMeshStaticSortKey(ToonOutlinePassShader.VertexShader, ToonOutlinePassShader.PixelShader);  
    //PassDrawRenderState.SetDepthStencilState(TStaticDepthStencilState<false, CF_DepthNearOrEqual>().GetRHI());  

    //FMeshPassProcessorRenderState DrawRenderState(PassDrawRenderState);  
    PassDrawRenderState.SetDepthStencilState(  
       TStaticDepthStencilState<  
       true, CF_GreaterEqual,// Enable DepthTest, It reverse about OpenGL(which is less)  
       false, CF_Never, SO_Keep, SO_Keep, SO_Keep,  
       false, CF_Never, SO_Keep, SO_Keep, SO_Keep,// enable stencil test when cull back  
       0x00,// disable stencil read  
       0x00>// disable stencil write  
       ::GetRHI());  
    PassDrawRenderState.SetStencilRef(0);  
    BuildMeshDrawCommands(  
       MeshBatch,       
       BatchElementMask,       
       PrimitiveSceneProxy,       
       MaterialRenderProxy,       
       MaterialResource,       
       PassDrawRenderState,  
       ToonOutlinePassShader,       
       MeshFillMode,       
       MeshCullMode,       
       SortKey,       
       EMeshPassFeatures::Default,  
       ShaderElementData);  
    return true;  
}

// Register Pass to Global Manager
void SetupToonOutlinePassState(FMeshPassProcessorRenderState& DrawRenderState)
{
	DrawRenderState.SetDepthStencilState(TStaticDepthStencilState<true, CF_LessEqual>::GetRHI());
}

FMeshPassProcessor* CreateToonOutlinePassProcessor(  
	ERHIFeatureLevel::Type FeatureLevel,  
	const FScene* Scene,  
	const FSceneView* InViewIfDynamicMeshCommand,  
	FMeshPassDrawListContext* InDrawListContext)  
{  
	FMeshPassProcessorRenderState ToonOutLinePassState;  
	SetupToonOutlinePassState(ToonOutLinePassState);  

	return new FToonOutlinePassProcessor(  
	   Scene,  
	   InViewIfDynamicMeshCommand,       
	   ToonOutLinePassState,       
	   InDrawListContext);  
}  

FRegisterPassProcessorCreateFunction RegisterToonOutlineMeshPass(  
	&CreateToonOutlinePassProcessor,  
	EShadingPath::Deferred,  
	EMeshPass::ToonOutlinePass,  
	EMeshPassFlags::CachedMeshCommands | EMeshPassFlags::MainView  
);
 
DECLARE_CYCLE_STAT(TEXT("ToonOutlinePass"), STAT_CLP_ToonOutlinePass, STATGROUP_SceneRendering);  

BEGIN_SHADER_PARAMETER_STRUCT(FToonOutlineMeshPassParameters, )  
    SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)  
    SHADER_PARAMETER_STRUCT_INCLUDE(FInstanceCullingDrawParams, InstanceCullingDrawParams)  
    RENDER_TARGET_BINDING_SLOTS()  
END_SHADER_PARAMETER_STRUCT()  

FToonOutlineMeshPassParameters* GetOutlinePassParameters(FRDGBuilder& GraphBuilder, const FViewInfo& View, FSceneTextures& SceneTextures)  
{  
    FToonOutlineMeshPassParameters* PassParameters = GraphBuilder.AllocParameters<FToonOutlineMeshPassParameters>();  
    PassParameters->View = View.ViewUniformBuffer;  

    PassParameters->RenderTargets[0] = FRenderTargetBinding(SceneTextures.Color.Target, ERenderTargetLoadAction::ELoad);  
    PassParameters->RenderTargets.DepthStencil = FDepthStencilBinding(SceneTextures.Depth.Target, ERenderTargetLoadAction::ELoad, ERenderTargetLoadAction::ELoad, FExclusiveDepthStencil::DepthWrite_StencilWrite);  

    return PassParameters;  
}  

void FDeferredShadingSceneRenderer::RenderToonOutlinePass(FRDGBuilder& GraphBuilder, FSceneTextures& SceneTextures)  
{  
    RDG_EVENT_SCOPE(GraphBuilder, "ToonOutlinePass");  
    RDG_CSV_STAT_EXCLUSIVE_SCOPE(GraphBuilder, RenderToonOutlinePass);  

    SCOPED_NAMED_EVENT(FDeferredShadingSceneRenderer_RenderToonOutlinePass, FColor::Emerald);  

    for(int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex)  
    {       
        FViewInfo& View = Views[ViewIndex];  
        RDG_GPU_MASK_SCOPE(GraphBuilder, View.GPUMask);  
        RDG_EVENT_SCOPE_CONDITIONAL(GraphBuilder, Views.Num() > 1, "View%d", ViewIndex);  

        const bool bShouldRenderView = View.ShouldRenderView();  
        if(bShouldRenderView)  
        {          
            FToonOutlineMeshPassParameters* PassParameters = GetOutlinePassParameters(GraphBuilder, View, SceneTextures);  

            View.ParallelMeshDrawCommandPasses[EMeshPass::ToonOutlinePass].BuildRenderingCommands(GraphBuilder, Scene->GPUScene, PassParameters->InstanceCullingDrawParams);  
            GraphBuilder.AddDispatchPass(  
                 RDG_EVENT_NAME("ToonOutlinePass"),  
                 PassParameters,             
                 ERDGPassFlags::Raster | ERDGPassFlags::SkipRenderPass,  
                 [&View, PassParameters](FRDGDispatchPassBuilder& DispatchPassBuilder)  
            {             
                View.ParallelMeshDrawCommandPasses[EMeshPass::ToonOutlinePass].Dispatch(DispatchPassBuilder, &PassParameters->InstanceCullingDrawParams);
            });
        }    
    }
}