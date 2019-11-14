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
 *	��4.17��
 *		Vertex Buffer��Ҫ��FVectorBuffer�̳У�Ȼ���Զ���
 *  4.19��
 *		Vertex Bufferʹ��FStaticMeshVertexBuffers VertexBuffers;
 *		���������й�Vertex Buffer�Ĳ������������ı䣬�·��һ��ʶ����
 *			ע��һ�����FStaticMeshVertexBuffers��������Buffer���������ºܶ����Ҫ�ֱ�����������в�����������Ե��ȥ��
 *  
 *  ����ο�CableComponent
 */

/* Index Buffer*/
// �������ԭ����û�䣬�²�֮��Ҳ��ĵ�
class FRayLineMeshIndexBuffer :public FIndexBuffer
{
public:
	//������ڸ���NumIndices�Ĵ�С��������һ��������������������
	virtual  void InitRHI() override
	{
		FRHIResourceCreateInfo CreateInfo;
		//IndexBufferRHI��FIndexBuffer�ĳ�Ա���ǿ��Բ²�FIndexBufferͬһʱ��ֻ�ܸ�һ��Component�����
		//BUF_Dynamic:The buffer will be written to occasionally, GPU read only, CPU write only.  The data lifetime is until the next update, or the buffer is destroyed.
		IndexBufferRHI = RHICreateIndexBuffer(sizeof(int32), NumIndices * sizeof(int32), BUF_Dynamic, CreateInfo);
	}

	int32 NumIndices;
};

/* Vertex Factory */
/**
 *	��4.17��
 *		Vertex Factory��Ҫ��FLocalVertexFactory�̳У�Ȼ���Զ���
 *  4.19��
 *		Vertex Factoryֱ��ʹ��FLocalVertexFactory VertexFactory;
 *		���������й�Vertex Factory�Ĳ������������ı䣬�·��һ��ʶ����
 * 	
 * 	����ο�CableComponent
 */

// Dynamic data sent to render thread
// ��������һ�����ݰ����������Ǵ��߼���������Ǵ��һ���͵���Ⱦ�߳�
struct FRayLineDynamicData
{
	// Array of points
	TArray<FVector> HitpointsPosition;
};

/* Scene Proxy */
// * Encapsulates the data which is mirrored to render a UPrimitiveComponent parallel to the game thread.
// * This is intended to be subclassed to support different primitive types.
// ����������FPrimitiveSceneProxy���������������������ʱ��̳�FPrimitiveSceneProxy���Զ���
class FRayLineMeshSceneProxy :public FPrimitiveSceneProxy
{
public:
	/** Return a type (or subtype) specific hash for sorting purposes */
	// 4.17�в�����д��4.19����Ҫ����ȫ�ճ����ɣ����岻��
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
		//VertexBuffer�ĳ�ʼ��4.17�汾
		//	VertexBuffer.NumVertices = GetRequiredVertexCount();
		//	�����Ϊ�·���ʼ�� ��һ������VertexBuffers����NumVertices��Stride
		VertexBuffers.InitWithDummyData(&VertexFactory, GetRequiredVertexCount());
		IndexBuffer.NumIndices = GetRequiredIndexCount();

		//VertexFactory�ĳ�ʼ��4.17�汾
		//	VertexFactory.Init(&VertexBuffer)
		//	�����ΪLINE 83�еĳ�ʼ��

		//Enqueue initialization of render resource
		//IndexBuffer�ĳ�ʼ��
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
	// ����������ڽ���Ҫ��Ⱦ��ģ�ͼ��뵽Collector�У�Collector.AddMesh���Խ���Ⱦ���ݼ������Ⱦ������ȥ
	// 	* @param Views - the array of views to consider.  These may not exist in the ViewFamily.
	//	* @param ViewFamily - the view family, for convenience
	//	* @param VisibilityMap - a bit representing this proxy's visibility in the Views array
	//	* @param Collector - gathers the mesh elements to be rendered and provides mechanisms for temporary allocations
	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
	{
		//����ʱ�йصĺ�
		QUICK_SCOPE_CYCLE_COUNTER(STAT_FRayLineMeshSceneProxy_GetDynamicMeshElements)

		//�Ƿ����߿�
		const bool bWireframe = AllowDebugViewmodes() && ViewFamily.EngineShowFlags.Wireframe;

		//������Ӧ�����ʵ��
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

	//��Primitive�������ã�������ݿ��Ե��ȥ��
	//�����ý�ʹ����Щ��Ⱦģ��
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
			
			//���������ݱ�����VertexBuffers��PositionVertexBuffer��Data��������
			VertexBuffers.PositionVertexBuffer.VertexPosition(i) = Vertex.Position;
			VertexBuffers.StaticMeshVertexBuffer.SetVertexTangents(i, Vertex.TangentX, Vertex.GetTangentY(), Vertex.TangentZ);
			VertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i, 0, Vertex.TextureCoordinate[0]);
			//���������ݱ�����VertexBuffers��PositionVertexBuffer��Data��������
			VertexBuffers.ColorVertexBuffer.VertexColor(i) = Vertex.Color;
		}

		{
			auto& VertexBuffer = VertexBuffers.PositionVertexBuffer;
			//VertexBuffer.VertexBufferRHI �²⣺����������dx������VertexBuffer
			//�²⣺����XXXRHI������������ֲ�ͬƽ̨��,����ҪDrawCall��ʹ�ñ���Ҫʹ��XXXRHI��������ЩXXXRHI���Զ�ƥ�䲻ͬ��ƽ̨���������ͼ�ο�(dx,opengl��)
			//�й�XXXRHI���Կ�һ�������http://www.manew.com/thread-100777-1-1.html
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

	//4.19�����������proxy����ڲ�������4.17�����ⲿ����proxy��
	FRayLineDynamicData* DynamicData;

	//The view relevance for all the static mesh's materials.
	FMaterialRelevance MaterialRelevance;
};

///////////////////////URayBasicComponent///////////////////////////////////////////////////////

//Constructor -> OnRegister-> [TickComponent -> CreateRenderState_Concurrent -> SendRenderDynamicData_Concurrent]

URayBasicComponent::URayBasicComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	//һ��Ҫ���������ش򿪣��������ܵ���ComponentTick��Щ�������������ǵ������
	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = true;
	bAutoActivate = true;

	DebugSec = 200.f;
}

//��ʼ��RayLineHitPoints
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

	//��������Ὺ��һ�����أ�������ÿ֡�������������Ⱦ״̬��ʱ�򣬻���µ����ǵ������
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
	//��������Ὺ��һ�����أ�������ÿ֡�������������Ⱦ״̬��ʱ�򣬻���µ����ǵ������
	//���Զ�����CreateRenderState_Concurrent()��Ȼ�����SendRenderDynamicData_Concurrent()
	MarkRenderDynamicDataDirty();

	//call this because bounds have changed
	//��ʹ�õ�CalcBounds����
	UpdateComponentToWorld();	
}

//������������������������߼��̵߳����ݷ��͵���Ⱦ�̵߳ġ�
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
			//����������Զ���ĳ�������
			FRayLineMeshSceneProxy*, RayLineSceneProxy, (FRayLineMeshSceneProxy*)SceneProxy,
			//������Ⱦ�Ķ�������
			FRayLineDynamicData*,NewDynamicData,NewDynamicData,
			{
				RayLineSceneProxy->SetDynamicData_RenderThread(NewDynamicData);
			}
		)
	}
}

//�²⣺������Ӧ�����ڴ���Component��ʱ����Ҫ������������ز���
//������������
//�²�:��RenderScene.cpp��void FScene::AddPrimitive(UPrimitiveComponent* Primitive)�е���(LINE 847)
FPrimitiveSceneProxy* URayBasicComponent::CreateSceneProxy()
{
	return new FRayLineMeshSceneProxy(this);
}

//��ȡ����
int32 URayBasicComponent::GetNumMaterials() const
{
	return 1;
}

//������Χ��
//�²�:��RenderScene.cpp��void FScene::AddPrimitive(UPrimitiveComponent* Primitive)�е���(LINE 883)
FBoxSphereBounds URayBasicComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	FBoxSphereBounds NewBounds;
	NewBounds.Origin = FVector::ZeroVector;
	NewBounds.BoxExtent = FVector(HALF_WORLD_MAX, HALF_WORLD_MAX, HALF_WORLD_MAX);
	NewBounds.SphereRadius = FMath::Sqrt(3.f*FMath::Square(HALF_WORLD_MAX));
	return NewBounds;
}






