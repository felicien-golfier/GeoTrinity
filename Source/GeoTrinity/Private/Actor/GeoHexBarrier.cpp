// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/GeoHexBarrier.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Curves/CurveVector.h"
#include "Tool/UGeoGameplayLibrary.h"

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
	float const PreviousLerpAlpha = LerpAlpha;
	Super::Tick(DeltaSeconds);
	ApplyTileVisuals(/*bRemoving*/ LerpAlpha > PreviousLerpAlpha);
}

void AGeoHexBarrier::ApplyTileVisuals(bool const bRemoving)
{
	int32 const NumTiles = NumColumns * NumRows;
	int32 const HiddenCount = FMath::RoundToInt32(LerpAlpha * NumTiles);

	if (HiddenCount != AppliedHiddenCount)
	{
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

	// All still-visible tiles shake together the instant removal starts, so their imminent vanish reads clearly.
	if (bRemoving && HiddenCount < NumTiles && ShakeCurve)
	{
		FVector const ShakeParams = ShakeCurve->GetVectorValue(LerpAlpha);
		float const CurrentTime = GetWorld()->GetTimeSeconds();
		for (int32 Index = HiddenCount; Index < NumTiles; ++Index)
		{
			float const RandomPhase = FMath::FRandRange(0.f, 2.f * PI);
			float const ShakeOffset = FMath::Sin(CurrentTime * ShakeParams.Y + RandomPhase) * ShakeParams.X;
			FTransform ShakeTransform = GetTileTransform(Index);
			ShakeTransform.AddToTranslation(FVector(ShakeOffset, 0.f, 0.f));
			TileMeshComponent->UpdateInstanceTransform(Index, ShakeTransform, /*bWorldSpace*/ false,
													   /*bMarkRenderStateDirty*/ false, /*bTeleport*/ true);
		}
		TileMeshComponent->MarkRenderStateDirty();
		bWasShaking = true;
	}
	else if (bWasShaking)
	{
		for (int32 Index = HiddenCount; Index < NumTiles; ++Index)
		{
			TileMeshComponent->UpdateInstanceTransform(Index, GetTileTransform(Index), /*bWorldSpace*/ false,
													   /*bMarkRenderStateDirty*/ false, /*bTeleport*/ true);
		}
		TileMeshComponent->MarkRenderStateDirty();
		bWasShaking = false;
	}
}
