// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.  

#include "MyShaderTest.h"  

#include "Classes/Engine/TextureRenderTarget2D.h"  
#include "Classes/Engine/World.h"  
#include "Public/GlobalShader.h"  
#include "Public/PipelineStateCache.h"  
#include "Public/RHIStaticStates.h"  
#include "Public/SceneUtils.h"  
#include "Public/SceneInterface.h"  
#include "Public/ShaderParameterUtils.h"  
#include "Public/Logging/MessageLog.h"  
#include "Public/Internationalization/Internationalization.h"  
#include "Public/StaticBoundShaderState.h"
#include "HAL/FileManager.h"

#include "RHICommandList.h"
#include "UniformBuffer.h"
#include "Engine/Texture2D.h"
#include "FileHelper.h"

#define LOCTEXT_NAMESPACE "TestShader"

BEGIN_UNIFORM_BUFFER_STRUCT(FMyUniformStructData, )
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, ColorOne)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, ColorTwo)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, ColorThree)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, ColorFour)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(uint32, ColorIndex)
END_UNIFORM_BUFFER_STRUCT(FMyUniformStructData)
//FMyUniformStructData用于C++文件，FMyUniform用于HLSL文件
IMPLEMENT_UNIFORM_BUFFER_STRUCT(FMyUniformStructData, TEXT("FMyUniform"));

//typedef TUniformBufferRef<MyStructData> MyStructDataRef;  

UTestShaderBlueprintLibrary::UTestShaderBlueprintLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

class FMyShaderTest : public FGlobalShader
{
public:

	FMyShaderTest() {}

	FMyShaderTest(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		SimpleColorVal.Bind(Initializer.ParameterMap, TEXT("SimpleColor"));
		TestTextureVal.Bind(Initializer.ParameterMap, TEXT("MyTexture"));
		TestTextureSampler.Bind(Initializer.ParameterMap, TEXT("MyTextureSampler"));
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return true;
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		//return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM4);  
		return true;
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("TEST_MICRO"), 1);
	}

	//
	void SetParameters(
		FRHICommandListImmediate& RHICmdList,
		const FLinearColor &MyColor,
		FTextureRHIParamRef &MyTexture,
		FMyShaderStructData &ShaderStructData
	)
	{
		SetShaderValue(RHICmdList, GetPixelShader(), SimpleColorVal, MyColor);
		SetTextureParameter(
			RHICmdList, 
			GetPixelShader(),
			TestTextureVal, 
			TestTextureSampler,
			TStaticSamplerState<SF_Trilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI(),
			MyTexture);

		FMyUniformStructData UniformData;
		UniformData.ColorOne = ShaderStructData.ColorOne;
		UniformData.ColorTwo = ShaderStructData.ColorTwo;
		UniformData.ColorThree = ShaderStructData.ColorThree;
		UniformData.ColorFour = ShaderStructData.ColorFour;
		UniformData.ColorIndex = ShaderStructData.ColorIndex;

		SetUniformBufferParameterImmediate(RHICmdList, GetPixelShader(), GetUniformBufferParameter<FMyUniformStructData>(), UniformData);
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << SimpleColorVal << TestTextureVal;
		return bShaderHasOutdatedParameters;
	}

private:

	FShaderParameter SimpleColorVal;
	FShaderResourceParameter TestTextureVal;
	FShaderResourceParameter TestTextureSampler;

};

class FShaderTestVS : public FMyShaderTest
{
	DECLARE_SHADER_TYPE(FShaderTestVS, Global);

public:
	FShaderTestVS() {}

	FShaderTestVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FMyShaderTest(Initializer)
	{

	}
};


class FShaderTestPS : public FMyShaderTest
{
	DECLARE_SHADER_TYPE(FShaderTestPS, Global);

public:
	FShaderTestPS() {}

	FShaderTestPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FMyShaderTest(Initializer)
	{

	}
};

IMPLEMENT_SHADER_TYPE(, FShaderTestVS, TEXT("/Plugin/ShadertestPlugin/Private/MyShader.usf"), TEXT("MainVS"), SF_Vertex)
IMPLEMENT_SHADER_TYPE(, FShaderTestPS, TEXT("/Plugin/ShadertestPlugin/Private/MyShader.usf"), TEXT("MainPS"), SF_Pixel)

struct FMyTextureVertex
{
	FVector4 Position;
	FVector2D UV;
};

class FMyTextureVertexDeclaration:public FRenderResource
{
public:
	FVertexDeclarationRHIRef VertexDeclarationRHI;

	virtual void InitRHI() override
	{
		FVertexDeclarationElementList Elements;
		uint32 Stride = sizeof(FMyTextureVertex);
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FMyTextureVertex, Position), VET_Float4, 0, Stride));
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FMyTextureVertex, UV), VET_Float2, 1, Stride));
		VertexDeclarationRHI = RHICreateVertexDeclaration(Elements);
	}

	virtual void ReleaseRHI() override
	{
		VertexDeclarationRHI->Release();
	}
};

static void DrawTestShaderRenderTarget_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	FTextureRenderTargetResource* OutputRenderTargetResource,
	ERHIFeatureLevel::Type FeatureLevel,
	FName TextureRenderTargetName,
	FLinearColor MyColor,
	FTextureRHIParamRef MyTexture,
	FMyShaderStructData ShaderStructData
)
{
	check(IsInRenderingThread());

#if WANTS_DRAW_MESH_EVENTS  
	FString EventName;
	TextureRenderTargetName.ToString(EventName);
	SCOPED_DRAW_EVENTF(RHICmdList, SceneCapture, TEXT("ShaderTest %s"), *EventName);
#else  
	SCOPED_DRAW_EVENT(RHICmdList, DrawUVDisplacementToRenderTarget_RenderThread);
#endif  

	//设置渲染目标  
	SetRenderTarget(
		RHICmdList,
		OutputRenderTargetResource->GetRenderTargetTexture(),
		FTextureRHIRef(),
		ESimpleRenderTargetMode::EUninitializedColorAndDepth,
		FExclusiveDepthStencil::DepthNop_StencilNop
	);

	//设置视口  
	//FIntPoint DrawTargetResolution(OutputRenderTargetResource->GetSizeX(), OutputRenderTargetResource->GetSizeY());  
	//RHICmdList.SetViewport(0, 0, 0.0f, DrawTargetResolution.X, DrawTargetResolution.Y, 1.0f);  

	TShaderMap<FGlobalShaderType>* GlobalShaderMap = GetGlobalShaderMap(FeatureLevel);
	TShaderMapRef<FShaderTestVS> VertexShader(GlobalShaderMap);
	TShaderMapRef<FShaderTestPS> PixelShader(GlobalShaderMap);

	FMyTextureVertexDeclaration VertexDec;
	VertexDec.InitRHI();

	// Set the graphic pipeline state.  
	FGraphicsPipelineStateInitializer GraphicsPSOInit;
	RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
	GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
	GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
	GraphicsPSOInit.PrimitiveType = PT_TriangleList;
	GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = VertexDec.VertexDeclarationRHI;
	//GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GetVertexDeclarationFVector4();
	GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
	GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
	SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

	//RHICmdList.SetViewport(0, 0, 0.0f, DrawTargetResolution.X, DrawTargetResolution.Y, 1.0f);  
	PixelShader->SetParameters(RHICmdList, MyColor,MyTexture,ShaderStructData);

	// Draw grid.  
	//uint32 PrimitiveCount = 2;  
	//RHICmdList.DrawPrimitive(PT_TriangleList, 0, PrimitiveCount, 1);  
	FMyTextureVertex Vertices[4];
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
	//DrawPrimitiveUP(RHICmdList, PT_TriangleStrip, 2, Vertices, sizeof(Vertices[0]));  
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

	// Resolve render target.  
	RHICmdList.CopyToResolveTarget(
		OutputRenderTargetResource->GetRenderTargetTexture(),
		OutputRenderTargetResource->TextureRHI,
		false, FResolveParams());
}

void UTestShaderBlueprintLibrary::DrawTestShaderRenderTarget(
	UTextureRenderTarget2D* OutputRenderTarget,
	AActor* Ac,
	FLinearColor MyColor,
	UTexture* MyTexture,
	FMyShaderStructData ShaderStructData
)
{
	check(IsInGameThread());

	if (!OutputRenderTarget)
	{
		return;
	}

	FTextureRenderTargetResource* TextureRenderTargetResource = OutputRenderTarget->GameThread_GetRenderTargetResource();
	FTextureRHIParamRef MyTextureRHI = MyTexture->TextureReference.TextureReferenceRHI;
	UWorld* World = Ac->GetWorld();
	ERHIFeatureLevel::Type FeatureLevel = World->Scene->GetFeatureLevel();
	FName TextureRenderTargetName = OutputRenderTarget->GetFName();
	ENQUEUE_RENDER_COMMAND(CaptureCommand)(
		[TextureRenderTargetResource, FeatureLevel, MyColor, TextureRenderTargetName,MyTextureRHI,ShaderStructData](FRHICommandListImmediate& RHICmdList)
	{
		DrawTestShaderRenderTarget_RenderThread(RHICmdList, TextureRenderTargetResource, FeatureLevel, TextureRenderTargetName, MyColor,MyTextureRHI,ShaderStructData);
	}
	);

}

static void TextureWriting_RenderThread(FRHICommandListImmediate& RHICmdList,ERHIFeatureLevel::Type FeatureLevel, UTexture2D* Texture)
{
	check(IsInRenderingThread());
	if (Texture == nullptr) return;

	//从UTexture2D转换为FTexture2D
	FTextureReferenceRHIRef MyTextureRHI = Texture->TextureReference.TextureReferenceRHI;
	FRHITexture* TexRef = MyTextureRHI->GetTextureReference()->GetReferencedTexture();
	FRHITexture2D* TexRef2D = (FRHITexture2D*)TexRef;

	
	//Bitmap用于存储从FRHITexture2D读取的颜色信息（32bit）
	TArray<FColor> Bitmap;
	//TextureDataPtr为FRHITexture2D数据块首地址,LolStride用于跳转到FRHITexture2D的下一行
	uint32 LolStride = 0;
	char* TextureDataPtr = (char*)RHICmdList.LockTexture2D(TexRef2D,0,EResourceLockMode::RLM_ReadOnly,LolStride,false);

	//获取FRHITexture2D每个像素的颜色信息并存储到Bitmap
	for(uint32 Row =0; Row < TexRef2D->GetSizeY();++Row)
	{
		//注意遍历的是地址，这里的强制类型转换不会改变指针值，只改变位数。
		//64位机指针都是8个字节，这里为什么要（char*） TextureDataPtr再转换成(uint32*)TextureDataPtr，原因未知
		uint32* PixelPtr = (uint32*)TextureDataPtr;
		for(uint32 Col = 0; Col < TexRef2D->GetSizeX();++Col)
		{
			//这里才取得地址对应的像素颜色
			uint32 EncodePixel = *PixelPtr;
			uint8 r = (EncodePixel & 0x000000FF);
			uint8 g = (EncodePixel & 0x0000FF00)>>8;
			uint8 b = (EncodePixel & 0x00FF0000)>>16;
			uint8 a = (EncodePixel & 0xFF000000)>>24;
			Bitmap.Add(FColor(b, g, r, a));
			//这里的++实际增加了8个字节
			PixelPtr++;
		}
		//move to next row
		TextureDataPtr += LolStride;
	}
	RHICmdList.UnlockTexture2D(TexRef2D, 0, false);

	if(Bitmap.Num())
	{
		IFileManager::Get().MakeDirectory(*FPaths::ScreenShotDir(), true);
		const FString ScreenFileName(FPaths::ScreenShotDir() / TEXT("VisualizeTexture"));
		//疑惑？ Bitmap.Num()/ Texture->GetSizeY() 跟 Texture->GetSizeX（）不一样吗？
		uint32  ExtendXWithMSAA = Bitmap.Num()/ Texture->GetSizeY();
		//Save the contents of the array to a bitmap file(24bit only so alpha channel is dropped)
		FFileHelper::CreateBitmap(*ScreenFileName, ExtendXWithMSAA, Texture->GetSizeY(), Bitmap.GetData());
		UE_LOG(LogConsoleResponse, Display, TEXT("Content was saved to \"%s\""), *FPaths::ScreenShotDir());
	}
	else
	{
		UE_LOG(LogConsoleResponse, Display, TEXT("Failed to save BMP,format or texture is not supported"));
	}
}

void UTestShaderBlueprintLibrary::TextureWriting(UTexture2D* TextureToBeWrite, AActor* selfref)
{
	//check(IsInGameThread());

	//if (selfref == nullptr && TextureToBeWrite == nullptr) return;
	////压缩设置为RGBA(8)
	//TextureToBeWrite->CompressionSettings = TextureCompressionSettings::TC_VectorDisplacementmap;
	//TextureToBeWrite->SRGB = 0;
	////获取材质的mipmap信息，并锁定数据块得到一个指向它的指针
	//FTexture2DMipMap& mipmap = TextureToBeWrite->PlatformData->Mips[0];
	//void* Data = mipmap.BulkData.Lock(LOCK_READ_WRITE);
	////获取材质的长宽
	//int32 textureX = TextureToBeWrite->PlatformData->SizeX;
	//int32 textureY = TextureToBeWrite->PlatformData->SizeY;

	////向colors中填充TextureToBeWrite需要的颜色信息
	//TArray<FColor> colors;
	//for(int32 x = 0; x < textureX*textureY ;++x)
	//{
	//	colors.Add(FColor::Blue);
	//}
	////每个颜色信息的大小
	//int32 stride = (int32)(sizeof(uint8) * 4);

	////复制数据到数据内存位置。(数据块指针，待用数据数组首指针，待用数组总大小）
	//FMemory::Memcpy(Data, colors.GetData(), textureX*textureY*stride);
	////解锁数据块
	//mipmap.BulkData.Unlock();
	////更新材质数据
	//TextureToBeWrite->UpdateResource();

	UWorld* World = selfref->GetWorld();
	ERHIFeatureLevel::Type FeatureLevel = World->Scene->GetFeatureLevel();

	ENQUEUE_RENDER_COMMAND(CaptureCommand)(
		[FeatureLevel,TextureToBeWrite](FRHICommandListImmediate& RHICmdList)
	{
		TextureWriting_RenderThread
		(
			RHICmdList,
			FeatureLevel,
			TextureToBeWrite
		);
	}
	);
	
}

#undef LOCTEXT_NAMESPACE  
