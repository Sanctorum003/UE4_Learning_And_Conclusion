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
	
	//构造函数
	FRayLineHitPointDesc() :
		HitPosition(NULL),
		HitNextDir(NULL),
		HitPointIndex(NULL)
	{

	}
};

UCLASS(hideCategories = (Object, LOD, Physics, Collision), editinlinenew, meta = (BlueprintSpawnableComponent), ClassGroup = Rendering)
//RAYLINE_API是导出宏用于dll库导出，跟我们现在做的东西关系不大，可忽略
class RAYLINE_API URayBasicComponent :public UMeshComponent
{
	//注意这里使用GENERATED_BODY()得直接写构造函数
	//使用GENERATED_UCLASS_BODY()这其中已经包含构造函数
	//GENERATED_UCLASS_BODY()
	GENERATED_BODY()

public:

	URayBasicComponent(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RayLineComponent)
		float DebugSec;

private:

	//~ Begin UPrimitiveComponent Interface.
	//这个函数创建场景代理，场景代理的作用就是负责在渲染线程端把逻辑线程这边的数据压入渲染管线
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