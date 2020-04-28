#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Components/MeshComponent.h"
#include "RayBasicComponent.generated.h"

class FPrimitiveSceneProxy;

//
USTRUCT(BlueprintType)
struct FRayLineHitPointDesc
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RayLineHitPoint)
		FVector HitPosition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RayLineHitPoint)
		FVector HitNextDir;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RayLineHitPoint)
		int32 HitPointIndex;
	
	//���캯��
	FRayLineHitPointDesc() :
		HitPosition(NULL),
		HitNextDir(NULL),
		HitPointIndex(NULL)
	{

	}
};

UCLASS(hideCategories = (Object, LOD, Physics, Collision), editinlinenew, meta = (BlueprintSpawnableComponent), ClassGroup = Rendering)
//RAYLINE_API�ǵ���������dll�⵼�����������������Ķ�����ϵ���󣬿ɺ���
class RAYLINE_API URayBasicComponent :public UMeshComponent
{
	//ע������ʹ��GENERATED_BODY()��ֱ��д���캯��
	//ʹ��GENERATED_UCLASS_BODY()�������Ѿ��������캯��
	//GENERATED_UCLASS_BODY()
	GENERATED_BODY()

public:

	URayBasicComponent(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RayLineComponent)
		float DebugSec;

private:

	//~ Begin UPrimitiveComponent Interface.
	//�������������������������������þ��Ǹ�������Ⱦ�̶߳˰��߼��߳���ߵ�����ѹ����Ⱦ����
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	//~ End UPrimitiveComponent Interface.

	//~ Begin UMeshComponent Interface.
	virtual int32 GetNumMaterials() const override;
	//~ End UMeshComponent Interface.

	//~ Begin USceneComponent Interface.
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	//~ Begin USceneComponent Interface.

	//~ Begin UActorComponent Interface.
	virtual void OnRegister() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction) override;
	virtual void SendRenderDynamicData_Concurrent() override;
	virtual void CreateRenderState_Concurrent() override;
	//~ Begin UActorComponent Interface.

	TArray<FRayLineHitPointDesc> RayLineHitPoints;

	friend class FRayLineMeshSceneProxy;
};