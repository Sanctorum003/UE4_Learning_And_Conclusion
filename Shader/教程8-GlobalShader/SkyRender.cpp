#include "SkyRender.h"

#include "CoreMinimal.h"

#include "SceneRendering.h"
#include "RHICommandList.h"
#include "Shader.h"
#include "RHIStaticStates.h"
#include "ScenePrivate.h"

template<uint32 SampleLevel>
class TSkyRenderVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(TSkyRenderVS, Global, /*MYMODULE_API*/);

private:

	FShaderParameter Unity_VP;
public:

	TSkyRenderVS() {}
	TSkyRenderVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		Unity_VP.Bind(Initializer.ParameterMap, TEXT("UnityVP"));
		//VertexOffset.Bind(Initializer.ParameterMap, TEXT("VertexOffset"));
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("SAMPLELEVEL"), SampleLevel);
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return true;
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);

		Ar << Unity_VP;

		return bShaderHasOutdatedParameters;
	}

	void SetMatrices(FRHICommandListImmediate& RHICmdList, const FScene *Scene, const FViewInfo *View)
	{
		SetShaderValue(RHICmdList, GetVertexShader(), Unity_VP, View->ViewMatrices.GetViewProjectionMatrix());
	}
};
IMPLEMENT_SHADER_TYPE(template<>, TSkyRenderVS<0>, TEXT("/Engine/Private/SkyRender.usf"), TEXT("MainVS"), SF_Vertex);
IMPLEMENT_SHADER_TYPE(template<>, TSkyRenderVS<1>, TEXT("/Engine/Private/SkyRender.usf"), TEXT("MainVS"), SF_Vertex);
IMPLEMENT_SHADER_TYPE(template<>, TSkyRenderVS<2>, TEXT("/Engine/Private/SkyRender.usf"), TEXT("MainVS"), SF_Vertex);

template<uint32 SampleLevel>
class TSkyRenderPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(TSkyRenderPS, Global, /*MYMODULE_API*/);

	TSkyRenderPS() {}
	TSkyRenderPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		TestColor.Bind(Initializer.ParameterMap, TEXT("TestColor"));
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("SAMPLELEVEL"), SampleLevel);
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return true;
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		return bShaderHasOutdatedParameters;
	}

	void SetUniforms(FRHICommandList& RHICmdList, const FScene *Scene, const FSceneView *SceneView)
	{
		SetShaderValue(RHICmdList, GetPixelShader(), TestColor, FVector(1, 0, 0));
	}

private:
	void SetTexture(FRHICommandList& RHICmdList, UTexture2D *noisetex)
	{

	}

	FShaderParameter TestColor;
};
IMPLEMENT_SHADER_TYPE(template<>, TSkyRenderPS<0>, TEXT("/Engine/Private/SkyRender.usf"), TEXT("MainPS"), SF_Pixel);
IMPLEMENT_SHADER_TYPE(template<>, TSkyRenderPS<1>, TEXT("/Engine/Private/SkyRender.usf"), TEXT("MainPS"), SF_Pixel);
IMPLEMENT_SHADER_TYPE(template<>, TSkyRenderPS<2>, TEXT("/Engine/Private/SkyRender.usf"), TEXT("MainPS"), SF_Pixel);

FDebugPane DebugMesh;

template<uint32 samplvl>
void RenderInternal(
	FRHICommandList& RHICmdList,
	const FScene *Scene,
	const TArrayView<const FViewInfo *> PassViews,
	TShaderMap<FGlobalShaderType>* ShaderMap
)
{

}

void FSceneRenderer::RenderMyMesh(FRHICommandListImmediate& RHICmdList, const TArrayView<const FViewInfo *> PassViews, int32 sgPipeLineQuality)
{
	check(IsInRenderingThread());

	TShaderMap<FGlobalShaderType>* ShaderMap = GetGlobalShaderMap(FeatureLevel);

	// pso init
	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(RHICmdList);
	SceneContext.BeginRenderingSceneColor(RHICmdList, ESimpleRenderTargetMode::EExistingColorAndDepth, FExclusiveDepthStencil::DepthRead_StencilWrite, true);

	FGraphicsPipelineStateInitializer PSOInit;
	RHICmdList.ApplyCachedRenderTargets(PSOInit);

	PSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_None, false, false>::GetRHI();
	PSOInit.BlendState = TStaticBlendState<>::GetRHI();
	PSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_GreaterEqual>::GetRHI();
	PSOInit.PrimitiveType = EPrimitiveType::PT_TriangleList;
	PSOInit.BoundShaderState.VertexDeclarationRHI = GetVertexDeclarationFVector3();

	static const uint32 SampleLevel = 2;
	TShaderMapRef<TSkyRenderVS<SampleLevel>> Vs(ShaderMap);
	TShaderMapRef<TSkyRenderPS<SampleLevel>> Ps(ShaderMap);
	PSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*Vs);
	PSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*Ps);

	SetGraphicsPipelineState(RHICmdList, PSOInit);


	for (int i = 0; i < PassViews.Num(); ++i)
	{
		const FViewInfo *ViewInfo = PassViews[i];

		Ps->SetUniforms(RHICmdList, Scene, ViewInfo);

		Vs->SetMatrices(RHICmdList, Scene, ViewInfo);

		if (!DebugMesh.Initialized)
		{
			DebugMesh.Init();
		}

		RHICmdList.SetStreamSource(0, DebugMesh.VertexBufferRHI, 0);
		RHICmdList.DrawIndexedPrimitive(DebugMesh.IndexBufferRHI, PT_TriangleList, 0, 0, DebugMesh.VertexCount, 0, DebugMesh.PrimitiveCount, 1);
	}
}

FDebugPane::FDebugPane()
{
	Initialized = false;
}

FDebugPane::~FDebugPane()
{
	VertexBufferRHI.SafeRelease();
	IndexBufferRHI.SafeRelease();
}

void FDebugPane::EmptyRawData()
{
	VerBuffer.Empty();
	InBuffer.Empty();
}

void FDebugPane::Init()
{
	FillRawData();

	VertexCount = static_cast<uint32>(VerBuffer.Num());
	PrimitiveCount = static_cast<uint32>(InBuffer.Num() / 3);

	//GPU Vertex Buffer
	{
		TStaticMeshVertexData<FVector> VertexData(false);
		Stride = VertexData.GetStride();

		VertexData.ResizeBuffer(VerBuffer.Num());

		uint8* Data = VertexData.GetDataPointer();
		const uint8* InData = (const uint8*)&(VerBuffer[0]);
		FMemory::Memcpy(Data, InData, Stride * VerBuffer.Num());

		FResourceArrayInterface *ResourceArray = VertexData.GetResourceArray();
		FRHIResourceCreateInfo CreateInfo(ResourceArray);
		VertexBufferRHI = RHICreateVertexBuffer(ResourceArray->GetResourceDataSize(), BUF_Static, CreateInfo);
	}

	{
		TResourceArray<uint16, INDEXBUFFER_ALIGNMENT> IndexBuffer;
		IndexBuffer.AddUninitialized(InBuffer.Num());
		FMemory::Memcpy(IndexBuffer.GetData(), (void*)(&(InBuffer[0])), InBuffer.Num() * sizeof(uint16));

		// Create index buffer. Fill buffer with initial data upon creation
		FRHIResourceCreateInfo CreateInfo(&IndexBuffer);
		IndexBufferRHI = RHICreateIndexBuffer(sizeof(uint16), IndexBuffer.GetResourceDataSize(), BUF_Static, CreateInfo);
	}

	EmptyRawData();

	Initialized = true;
}