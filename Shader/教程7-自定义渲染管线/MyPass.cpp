#include "MyPass.h"
#include "DeferredShadingRenderer.h"
#include "AtmosphereRendering.h"
#include "ScenePrivate.h"
#include "Engine/TextureCube.h"
#include "PipelineStateCache.h"

DECLARE_GPU_STAT(MyPass);
//顶点着色器 
class FMyPassVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FMyPassVS, Global);
public:
	FMyPassVS(){}
	FMyPassVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):FGlobalShader(Initializer)
	{
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM4);
	}

	
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters,FShaderCompilerEnvironment& OutEnvironment)
	{
		
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		return bShaderHasOutdatedParameters;
	}
	
private:
};
//实例化顶点着色器
IMPLEMENT_SHADER_TYPE(, FMyPassVS, TEXT("/Engine/Private/MyPass.usf"), TEXT("MainVS"), SF_Vertex);
//像素着色器
class FMyPassPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FMyPassPS, Global);
public:
	FMyPassPS() {}
	FMyPassPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):FGlobalShader(Initializer)
	{
		
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4);;
	}
	
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM4);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{

	}
	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		return bShaderHasOutdatedParameters;
	}
	
private:

};
//全局实例化像素着色器
IMPLEMENT_SHADER_TYPE(, FMyPassPS, TEXT("/Engine/Private/MyPass.usf"), TEXT("MainPS"), SF_Pixel);

//顶点数据结构体
struct FMyPassVertex
{
	FVector4	Position;
	FVector2D	UV;
};

//顶点输入布局
class FMyPassVertexDec : public FRenderResource
{
public:
	FVertexDeclarationRHIRef VertexDeclarationRHI;

	virtual ~FMyPassVertexDec(){}

	virtual void InitRHI() override
	{
		FVertexDeclarationElementList Elements;
		uint32 Stride = sizeof(FMyPassVertex);
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FMyPassVertex, Position), VET_Float4, 0, Stride));
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FMyPassVertex, UV), VET_Float2, 1, Stride));
		VertexDeclarationRHI = RHICreateVertexDeclaration(Elements);
	}

	virtual void ReleaseRHI() override
	{
		VertexDeclarationRHI->Release();
	}
};

//顶点输入布局的全局实例，这么声明会自动init和release
TGlobalResource<FMyPassVertexDec> GMyPassVertexDeclaration;

//绘制函数
bool FDeferredShadingSceneRenderer::RenderMyPass(FRHICommandListImmediate& RHICmdList)
{
	FMyPassVertex Vertices[4];
	Vertices[0].Position.Set(-1.0f, 1.0f, 0, 1.0f);
	Vertices[1].Position.Set(1.0f, 1.0f, 0, 1.0f);
	Vertices[2].Position.Set(-1.0f, -1.0f, 0, 1.0f);
	Vertices[3].Position.Set(1.0f, -1.0f, 0, 1.0f);
	Vertices[0].UV = FVector2D(0.0f, 1.0f);
	Vertices[1].UV = FVector2D(1.0f, 1.0f);
	Vertices[2].UV = FVector2D(0.0f, 0.0f);
	Vertices[3].UV = FVector2D(1.0f, 0.0f);

	static const uint16 Indices[6] =
	{
		0, 1, 2,
		2, 1, 3
	};

	//获取场景的渲染目标
	//SceneRender会管理一帧画面的RT
	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(RHICmdList);
	//这句命令中会把最终的渲染目标设置为场景的RT
	SceneContext.BeginRenderingSceneColor(RHICmdList, ESimpleRenderTargetMode::EExistingColorAndDepth, FExclusiveDepthStencil::DepthRead_StencilWrite, true);

	FGraphicsPipelineStateInitializer GraphicsPSOInit;
	RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

	//处理游戏里所有的视口的
	for(int32 ViewIndex = 0; ViewIndex < Views.Num();++ViewIndex)
	{
		const FViewInfo& View = Views[ViewIndex];
		// 是否不是透视图
		if(View.IsPerspectiveProjection() == false)
		{
			continue;
		}

		//获取View.ShaderMap中类型为FMyPassVS的shader
		TShaderMapRef<FMyPassVS> VertexShader(View.ShaderMap);
		TShaderMapRef<FMyPassPS> PixelShader(View.ShaderMap);

		//怀疑是渲染的作用范围
		RHICmdList.SetViewport(
			View.ViewRect.Min.X,
			View.ViewRect.Min.Y,
			0,
			View.ViewRect.Max.X,
			View.ViewRect.Max.Y,
			1.f
		);		

		//GMyPassVertexDeclaration.InitRHI();
		
		GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
		GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGB, BO_Add, BF_One, BF_SourceAlpha>::GetRHI();//变了
		GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_None>::GetRHI();//变了
		GraphicsPSOInit.PrimitiveType = PT_TriangleList;
		GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GMyPassVertexDeclaration.VertexDeclarationRHI;
		GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
		GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
		SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

		DrawIndexedPrimitiveUP(
			RHICmdList,
			PT_TriangleList,
			0,
			ARRAY_COUNT(Vertices),
			2,
			Indices,
			sizeof(Indices[0]),
			Vertices,
			sizeof(Vertices[0])
		);
	}

	return true;
}