#include "RayBasicComponent.h"
#include "RenderingThread.h"
#include "RenderResource.h"
#include "PrimitiveViewRelevance.h"
#include "PrimitiveSceneProxy.h"
#include "VertexFactory.h"
#include "MaterialShared.h"
#include "Engine/CollisionProfile.h"
#include "Materials/Material.h"
#include "LocalVertexFactory.h"
#include "SceneManagement.h"
#include "DynamicMeshBuilder.h"
#include "EngineGlobals.h"
#include "Engine.h"
#include "Kismet/KismetSystemLibrary.h"

/* Vertex Buffer */
/**
 *	在4.17中
 *		Vertex Buffer需要从FVectorBuffer继承，然后自定义
 *  4.19中
 *		Vertex Buffer使用FStaticMeshVertexBuffers VertexBuffers;
 *		所以所有有关Vertex Buffer的操作都会有所改变，下方我会标识出来
 *			注意一点的是FStaticMeshVertexBuffers包含三个Buffer，所以以下很多操作要分别对这三个进行操作，具体可以点进去看
 *  
 *  具体参考CableComponent
 */

/* Index Buffer*/
// 这个还是原来的没变，猜测之后也会改掉
class FRayLineMeshIndexBuffer :public FIndexBuffer
{
public:
	//这个用于根据NumIndices的大小，来创建一个缓存区保存索引数据
	virtual  void InitRHI() override
	{
		FRHIResourceCreateInfo CreateInfo;
		//IndexBufferRHI是FIndexBuffer的成员，那可以猜测FIndexBuffer同一时刻只能给一个Component组件用
		//BUF_Dynamic:The buffer will be written to occasionally, GPU read only, CPU write only.  The data lifetime is until the next update, or the buffer is destroyed.
		IndexBufferRHI = RHICreateIndexBuffer(sizeof(int32), NumIndices * sizeof(int32), BUF_Dynamic, CreateInfo);
	}

	int32 NumIndices;
};

/* Vertex Factory */
/**
 *	在4.17中
 *		Vertex Factory需要从FLocalVertexFactory继承，然后自定义
 *  4.19中
 *		Vertex Factory直接使用FLocalVertexFactory VertexFactory;
 *		所以所有有关Vertex Factory的操作都会有所改变，下方我会标识出来
 * 	
 * 	具体参考CableComponent
 */

// Dynamic data sent to render thread
// 把它视作一个数据包，方便我们从逻辑层把数据们打包一起发送到渲染线程
struct FRayLineDynamicData
{
	// Array of points
	TArray<FVector> HitpointsPosition;
};

/* Scene Proxy */
// * Encapsulates the data which is mirrored to render a UPrimitiveComponent parallel to the game thread.
// * This is intended to be subclassed to support different primitive types.
// 上面两行是FPrimitiveSceneProxy的描述，创建场景代理的时候继承FPrimitiveSceneProxy并自定义
class FRayLineMeshSceneProxy :public FPrimitiveSceneProxy
{
public:
	/** Return a type (or subtype) specific hash for sorting purposes */
	// 4.17中不用重写，4.19中需要。完全照抄即可，意义不明
	SIZE_T GetTypeHash() const override
	{
		static size_t UniquePointer;
		return reinterpret_cast<size_t>(&UniquePointer);
	}

	//Constructor Func
	FRayLineMeshSceneProxy(URayBasicComponent* Component)
		: FPrimitiveSceneProxy(Component)
		, Material(NULL)
		, VertexFactory(GetScene().GetFeatureLevel(),"FCableSceneProxy")
		, DynamicData(nullptr)
		, MaterialRelevance(Component->GetMaterialRelevance(GetScene().GetFeatureLevel()))
	{
		//VertexBuffer的初始化4.17版本
		//	VertexBuffer.NumVertices = GetRequiredVertexCount();
		//	这里改为下方初始化 这一步会在VertexBuffers保存NumVertices和Stride
		VertexBuffers.InitWithDummyData(&VertexFactory, GetRequiredVertexCount());
		IndexBuffer.NumIndices = GetRequiredIndexCount();

		//VertexFactory的初始化4.17版本
		//	VertexFactory.Init(&VertexBuffer)
		//	这里改为LINE 83行的初始化

		//Enqueue initialization of render resource
		//IndexBuffer的初始化
		BeginInitResource(&IndexBuffer);

		//Grab material
		Material = Component->GetMaterial(0);
		if (Material == NULL)
			Material = UMaterial::GetDefaultMaterial(MD_Surface);
	}

	virtual ~FRayLineMeshSceneProxy()
	{
		VertexBuffers.PositionVertexBuffer.ReleaseResource();
		VertexBuffers.StaticMeshVertexBuffer.ReleaseResource();
		VertexBuffers.ColorVertexBuffer.ReleaseResource();
		IndexBuffer.ReleaseResource();
		VertexFactory.ReleaseResource();
	}

	int32 GetRequiredVertexCount() const
	{
		return 40;
	}

	int32 GetRequiredIndexCount() const
	{
		return 60;
	}
	
	// This function should not modify the proxy but simply collect a description of things to render.
	// 这个函数用于将需要渲染的模型加入到Collector中，Collector.AddMesh可以将渲染数据加入的渲染队列中去
	// 	* @param Views - the array of views to consider.  These may not exist in the ViewFamily.
	//	* @param ViewFamily - the view family, for convenience
	//	* @param VisibilityMap - a bit representing this proxy's visibility in the Views array
	//	* @param Collector - gathers the mesh elements to be rendered and provides mechanisms for temporary allocations
	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
	{
		//跟计时有关的宏
		QUICK_SCOPE_CYCLE_COUNTER(STAT_FRayLineMeshSceneProxy_GetDynamicMeshElements)

		//是否是线框
		const bool bWireframe = AllowDebugViewmodes() && ViewFamily.EngineShowFlags.Wireframe;

		//创建相应的相框实例
		auto WireframeMaterialInstance = new FColoredMaterialRenderProxy(
			GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy(IsSelected()) : NULL,
			FLinearColor(0, 0.5f, 1.f)
		);

		/** Add a material render proxy that will be cleaned up automatically */
		/** Material proxies that will be deleted at the end of the frame. */
		Collector.RegisterOneFrameMaterialProxy(WireframeMaterialInstance);

		FMaterialRenderProxy* MaterialProxy = NULL;
		if (bWireframe)
			MaterialProxy = WireframeMaterialInstance;
		else
			MaterialProxy = Material->GetRenderProxy(IsSelected());

		for(int32 ViewIndex = 0; ViewIndex < Views.Num();ViewIndex++)
		{
			if(VisibilityMap & (1<<ViewIndex))
			{
				const FSceneView* View = Views[ViewIndex];
				//Draw the mesh
				FMeshBatch& Mesh = Collector.AllocateMesh();
				FMeshBatchElement& BatchElement = Mesh.Elements[0];
				BatchElement.IndexBuffer = &IndexBuffer;
				Mesh.bWireframe = bWireframe;
				Mesh.VertexFactory = &VertexFactory;
				Mesh.MaterialRenderProxy = MaterialProxy;
				BatchElement.PrimitiveUniformBuffer = CreatePrimitiveUniformBufferImmediate(GetLocalToWorld(), GetBounds(), GetLocalBounds(), true, UseEditorDepthTest());
				BatchElement.FirstIndex = 0;
				BatchElement.NumPrimitives = GetRequiredIndexCount() / 3;
				BatchElement.MinVertexIndex = 0;
				BatchElement.MaxVertexIndex = GetRequiredVertexCount();
				Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
				Mesh.Type = PT_TriangleList;
				Mesh.DepthPriorityGroup = SDPG_World;
				/** Whether view mode overrides can be applied to this mesh eg unlit, wireframe. */
				Mesh.bCanApplyViewModeOverrides = false;
				Collector.AddMesh(ViewIndex, Mesh);
			}
		}
	}

	//对Primitive进行设置，相关内容可以点进去看
	//这设置将使用哪些渲染模型
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
	{
		FPrimitiveViewRelevance Result;
		Result.bDrawRelevance = IsShown(View);
		Result.bShadowRelevance = IsShadowCast(View);
		Result.bDynamicRelevance = true;
		Result.bRenderInMainPass = ShouldRenderCustomDepth();
		Result.bUsesLightingChannels = GetLightingChannelMask() != GetDefaultLightingChannelMask();
		Result.bRenderCustomDepth = ShouldRenderCustomDepth();
		MaterialRelevance.SetPrimitiveViewRelevance(Result);
		return Result;
	}

	//return true if the proxy can be culled when occluded by other primitives
	virtual bool CanBeOccluded() const override
	{
		return !MaterialRelevance.bDisableDepthTest;
	}

	virtual uint32 GetMemoryFootprint() const override
	{
		return (sizeof(*this) + GetAllocatedSize());
	}

	uint32 GetAllocatedSize(void) const
	{
		return (FPrimitiveSceneProxy::GetAllocatedSize());
	}

	//Called on render thread to assign new dynamic data
	void SetDynamicData_RenderThread(FRayLineDynamicData* NewDynamicData)
	{
		check(IsInRenderingThread());

		//Free existing data if present
		if(DynamicData)
		{
			delete DynamicData;
			DynamicData = NULL;
		}
		DynamicData = NewDynamicData;

		//Build mesh from HitpointsPosition
		TArray<FDynamicMeshVertex> Vertices;
		TArray<int32> Indices;
		BuildMesh(DynamicData->HitpointsPosition, Vertices, Indices);

		check(Vertices.Num() == GetRequiredVertexCount());
		check(Indices.Num() == GetRequiredIndexCount());

		for(int i = 0; i < Vertices.Num(); ++i)
		{
			const FDynamicMeshVertex& Vertex = Vertices[i];
			
			//将定点数据保存在VertexBuffers的PositionVertexBuffer的Data数据域中
			VertexBuffers.PositionVertexBuffer.VertexPosition(i) = Vertex.Position;
			VertexBuffers.StaticMeshVertexBuffer.SetVertexTangents(i, Vertex.TangentX, Vertex.GetTangentY(), Vertex.TangentZ);
			VertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i, 0, Vertex.TextureCoordinate[0]);
			//将定点数据保存在VertexBuffers的PositionVertexBuffer的Data数据域中
			VertexBuffers.ColorVertexBuffer.VertexColor(i) = Vertex.Color;
		}

		{
			auto& VertexBuffer = VertexBuffers.PositionVertexBuffer;
			//VertexBuffer.VertexBufferRHI 猜测：该是用来与dx交互的VertexBuffer
			//猜测：各种XXXRHI是用来抽象各种不同平台的,最终要DrawCall的使用必须要使用XXXRHI，这样这些XXXRHI会自动匹配不同的平台，调用相关图形库(dx,opengl等)
			//有关XXXRHI可以看一下这个：http://www.manew.com/thread-100777-1-1.html
			void* VertexBufferData = RHILockVertexBuffer(VertexBuffer.VertexBufferRHI, 0, VertexBuffer.GetNumVertices()* VertexBuffer.GetStride(), RLM_WriteOnly);
			//Dest,Src,count
			FMemory::Memcpy(VertexBufferData, VertexBuffer.GetVertexData(), VertexBuffer.GetNumVertices()*VertexBuffer.GetStride());
			RHIUnlockVertexBuffer(VertexBuffer.VertexBufferRHI);
		}

		{
			auto& VertexBuffer = VertexBuffers.ColorVertexBuffer;
			void* VertexBufferData = RHILockVertexBuffer(VertexBuffer.VertexBufferRHI, 0, VertexBuffer.GetNumVertices()*VertexBuffer.GetStride(), RLM_WriteOnly);
			FMemory::Memcpy(VertexBufferData, VertexBuffer.GetVertexData(), VertexBuffer.GetNumVertices()*VertexBuffer.GetStride());
			RHIUnlockVertexBuffer(VertexBuffer.VertexBufferRHI);
		}

		{
			auto& VertexBuffer = VertexBuffers.StaticMeshVertexBuffer;
			void* VertexBufferData = RHILockVertexBuffer(VertexBuffer.TangentsVertexBuffer.VertexBufferRHI, 0, VertexBuffer.GetTangentSize(), RLM_WriteOnly);
			FMemory::Memcpy(VertexBufferData, VertexBuffer.GetTangentData(), VertexBuffer.GetTangentSize());
			RHIUnlockVertexBuffer(VertexBuffer.TangentsVertexBuffer.VertexBufferRHI);
		}

		{
			auto& VertexBuffer = VertexBuffers.StaticMeshVertexBuffer;
			void* VertexBufferData = RHILockVertexBuffer(VertexBuffer.TexCoordVertexBuffer.VertexBufferRHI, 0, VertexBuffer.GetTexCoordSize(), RLM_WriteOnly);
			FMemory::Memcpy(VertexBufferData, VertexBuffer.GetTexCoordData(), VertexBuffer.GetTexCoordSize());
			RHIUnlockVertexBuffer(VertexBuffer.TexCoordVertexBuffer.VertexBufferRHI);
		}

		void* IndexBufferData = RHILockIndexBuffer(IndexBuffer.IndexBufferRHI, 0, Indices.Num() * sizeof(int32), RLM_WriteOnly);
		FMemory::Memcpy(IndexBufferData, &Indices[0], Indices.Num() * sizeof(int32));
		RHIUnlockIndexBuffer(IndexBuffer.IndexBufferRHI);
	}

	void BuildMesh(const TArray<FVector>& InPoints,TArray<FDynamicMeshVertex>& OutVertices,TArray<int32>& OutIndices)
	{
		for(int32 i = 0; i < InPoints.Num();++i)
		{
			FDynamicMeshVertex NewVertex[4];
			NewVertex[0].Position = InPoints[i] + FVector(-100, 100, 0);
			NewVertex[1].Position = InPoints[i] + FVector(100, 100, 0);
			NewVertex[2].Position = InPoints[i] + FVector(-100, -100, 0);
			NewVertex[3].Position = InPoints[i] + FVector(100, -100, 0);

			for(int i = 0; i < 4;++i)
			{
				OutVertices.Add(NewVertex[i]);
			}

			OutIndices.Add(4 * i);
			OutIndices.Add(4 * i+1);
			OutIndices.Add(4 * i+2);
			OutIndices.Add(4 * i+1);
			OutIndices.Add(4 * i+3);
			OutIndices.Add(4 * i+2);
			
		}
	}
	
private:
	UMaterialInterface * Material;
	FStaticMeshVertexBuffers VertexBuffers;
	FRayLineMeshIndexBuffer IndexBuffer;
	FLocalVertexFactory VertexFactory;

	//4.19中这个声明在proxy类的内部，不想4.17中是外部传入proxy类
	FRayLineDynamicData* DynamicData;

	//The view relevance for all the static mesh's materials.
	FMaterialRelevance MaterialRelevance;
};

///////////////////////URayBasicComponent///////////////////////////////////////////////////////

//Constructor -> OnRegister-> [TickComponent -> CreateRenderState_Concurrent -> SendRenderDynamicData_Concurrent]

URayBasicComponent::URayBasicComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	//一定要把三个开关打开，这样才能调用ComponentTick这些函数来更新我们的组件。
	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = true;
	bAutoActivate = true;

	DebugSec = 200.f;
}

//初始化RayLineHitPoints
void URayBasicComponent::OnRegister()
{
	Super::OnRegister();

	RayLineHitPoints.Reset();
	FVector RayDirection = FVector(1.f, 0.f, 0.f);
	FVector RayOrigin = FVector(0.f, 0.f, 0.f);
	int32 HitPointsNum = 10;
	float SecLength = 50.f;

	RayLineHitPoints.AddUninitialized(HitPointsNum);
	RayLineHitPoints[0].HitPosition = RayOrigin;
	RayLineHitPoints[0].HitNextDir = RayDirection;

	float t = DebugSec;
	for(int32 i = 1; i < HitPointsNum;++i)
	{
		RayLineHitPoints[i].HitPosition = RayDirection * t + RayOrigin;
		t += DebugSec;
	}

	//这个函数会开启一个开关，让引擎每帧更新所有组件渲染状态的时候，会更新到我们的组件。
	MarkRenderDynamicDataDirty();
}

/**
 * Function called every frame on this ActorComponent. Override this function to implement custom logic to be executed every frame.
 * Only executes if the component is registered, and also PrimaryComponentTick.bCanEverTick must be set to true.
 *
 * @param DeltaTime - The time since the last tick.
 * @param TickType - The kind of tick this is, for example, are we paused, or 'simulating' in the editor
 * @param ThisTickFunction - Internal tick function struct that caused this to run
 */
void URayBasicComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	RayLineHitPoints.Reset();
	FVector RayDirection = FVector(1.f, 0.f, 0.f);
	FVector RayOrigin = FVector(0.f, 0.f, 0.f);
	int32 HitPointsNum = 10;
	float SecLength = 50.f;

	RayLineHitPoints.AddUninitialized(HitPointsNum);
	RayLineHitPoints[0].HitPosition = RayOrigin;
	RayLineHitPoints[0].HitNextDir = RayDirection;

	float t = DebugSec;
	for (int32 i = 1; i < HitPointsNum; ++i)
	{
		RayLineHitPoints[i].HitPosition = RayDirection * t + RayOrigin;
		t += DebugSec;
	}

	//Need to send new data render thread
	//这个函数会开启一个开关，让引擎每帧更新所有组件渲染状态的时候，会更新到我们的组件。
	//会自动调用CreateRenderState_Concurrent()，然后调用SendRenderDynamicData_Concurrent()
	MarkRenderDynamicDataDirty();

	//call this because bounds have changed
	//有使用到CalcBounds（）
	UpdateComponentToWorld();	
}

//这两个函数才是真正负责把逻辑线程的数据发送到渲染线程的。
void URayBasicComponent::CreateRenderState_Concurrent()
{
	Super::CreateRenderState_Concurrent();

	SendRenderDynamicData_Concurrent();
}

void URayBasicComponent::SendRenderDynamicData_Concurrent()
{
	if(SceneProxy)
	{
		//Allocate Ray Line Dynamic Data
		FRayLineDynamicData* NewDynamicData = new FRayLineDynamicData;
		NewDynamicData->HitpointsPosition.AddUninitialized(RayLineHitPoints.Num());
		for (int32 i = 0; i < RayLineHitPoints.Num(); ++i)
			NewDynamicData->HitpointsPosition[i] = RayLineHitPoints[i].HitPosition;

		//Enqueue command to send to render thread
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			FSendRayBasicComponetDynamicData,
			//这个是我们自定义的场景代理
			FRayLineMeshSceneProxy*, RayLineSceneProxy, (FRayLineMeshSceneProxy*)SceneProxy,
			//我们渲染的顶点数据
			FRayLineDynamicData*,NewDynamicData,NewDynamicData,
			{
				RayLineSceneProxy->SetDynamicData_RenderThread(NewDynamicData);
			}
		)
	}
}

//猜测：这三个应该是在创建Component的时候需要调用来设置相关参数
//创建场景代理
//猜测:在RenderScene.cpp的void FScene::AddPrimitive(UPrimitiveComponent* Primitive)中调用(LINE 847)
FPrimitiveSceneProxy* URayBasicComponent::CreateSceneProxy()
{
	return new FRayLineMeshSceneProxy(this);
}

//获取材质
int32 URayBasicComponent::GetNumMaterials() const
{
	return 1;
}

//构建包围盒
//猜测:在RenderScene.cpp的void FScene::AddPrimitive(UPrimitiveComponent* Primitive)中调用(LINE 883)
FBoxSphereBounds URayBasicComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	FBoxSphereBounds NewBounds;
	NewBounds.Origin = FVector::ZeroVector;
	NewBounds.BoxExtent = FVector(HALF_WORLD_MAX, HALF_WORLD_MAX, HALF_WORLD_MAX);
	NewBounds.SphereRadius = FMath::Sqrt(3.f*FMath::Square(HALF_WORLD_MAX));
	return NewBounds;
}






