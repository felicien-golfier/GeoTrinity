// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/GeoHexBarrier.h"

#include "Components/InstancedStaticMeshComponent.h"

namespace
{
	constexpr float Sqrt3 = 1.7320508f;
} // namespace

AGeoHexBarrier::AGeoHexBarrier()
{
	TileMeshComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("TileMeshComponent"));
	SetRootComponent(TileMeshComponent);
	TileMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AGeoHexBarrier::OnConstruction(FTransform const& Transform)
{
	Super::OnConstruction(Transform);
	TileMeshComponent->ClearInstances();
	for (int32 Index = 0; Index < NumColumns * NumRows; ++Index)
	{
		TileMeshComponent->AddInstance(GetTileTransform(Index));
	}
	AppliedHiddenCount = 0;
}

FTransform AGeoHexBarrier::GetTileTransform(int32 const Index) const
{
	int32 const Column = bVanishAlongColumns ? Index / NumRows : Index % NumColumns;
	int32 const Row = bVanishAlongColumns ? Index % NumRows : Index / NumColumns;
	FVector const TileLocation(TileSize * Sqrt3 * (Column + 0.5f * (Row & 1)), TileSize * 1.5f * Row, 0.f);
	return FTransform(FRotator::ZeroRotator, TileLocation, FVector(TileSize / 100.f, TileSize / 100.f, 1.f));
}

void AGeoHexBarrier::Tick(float const DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ApplyTileVisuals();
}

void AGeoHexBarrier::ApplyTileVisuals()
{
	int32 const NumTiles = NumColumns * NumRows;
	int32 const HiddenCount = FMath::RoundToInt32(LerpAlpha * NumTiles);
	if (HiddenCount == AppliedHiddenCount)
	{
		return;
	}

	int32 const FirstChanged = FMath::Min(HiddenCount, AppliedHiddenCount);
	int32 const LastChanged = FMath::Max(HiddenCount, AppliedHiddenCount);
	for (int32 Index = FirstChanged; Index < LastChanged; ++Index)
	{
		FTransform TileTransform = GetTileTransform(Index);
		if (Index < HiddenCount)
		{
			TileTransform.SetScale3D(FVector::ZeroVector);
		}
		TileMeshComponent->UpdateInstanceTransform(Index, TileTransform, /*bWorldSpace*/ false,
												   /*bMarkRenderStateDirty*/ false, /*bTeleport*/ true);
	}
	AppliedHiddenCount = HiddenCount;
	TileMeshComponent->MarkRenderStateDirty();
}
