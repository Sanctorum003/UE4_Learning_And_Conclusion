#pragma once

#include "CoreMinimal.h"
#include "StaticMeshVertexDataInterface.h"
#include "StaticMeshVertexData.h"

#include "RenderResource.h"

struct FDebugPane
{
	FDebugPane();
	~FDebugPane();
	void FillRawData();
	void EmptyRawData();
	void Init();

	TArray<FVector> VerBuffer;
	TArray<uint16> InBuffer;

	uint32 Stride;

	bool Initialized;

	uint32 VertexCount;
	uint32 PrimitiveCount;

	FVertexBufferRHIRef VertexBufferRHI;
	FIndexBufferRHIRef IndexBufferRHI;
};

void FDebugPane::FillRawData()
{
	VerBuffer = {
		FVector(0.0f, 0.0f, 0.0f),
		FVector(100.0f, 0.0f, 0.0f),
		FVector(100.0f, 100.0f, 0.0f),
		FVector(0.0f, 100.0f, 0.0f)
	};

	InBuffer = {
		0, 1, 2,
		0, 2, 3
	};
}