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
	RebuildInstances();
}

void AGeoHexBarrier::BuildFullLayout()
{
	TileLayout.Reset(NumColumns * NumRows);
	for (int32 Index = 0; Index < NumColumns * NumRows; ++Index)
	{
		TileLayout.Emplace(bVanishAlongColumns ? Index / NumRows : Index % NumColumns,
						   bVanishAlongColumns ? Index % NumRows : Index / NumColumns);
	}
}

void AGeoHexBarrier::RebuildInstances()
{
	if (TileLayout.IsEmpty())
	{
		BuildFullLayout();
	}
	TileMeshComponent->ClearInstances();
	for (FIntPoint const Tile : TileLayout)
	{
		TileMeshComponent->AddInstance(GetTileTransform(Tile));
	}
	AppliedHiddenCount = 0;
}

void AGeoHexBarrier::CaptureLayout()
{
	TSet<FIntPoint> SurvivingTiles;
	FTransform InstanceTransform;
	for (int32 Index = 0; Index < TileMeshComponent->GetInstanceCount(); ++Index)
	{
		TileMeshComponent->GetInstanceTransform(Index, InstanceTransform);
		SurvivingTiles.Add(LocalToTile(InstanceTransform.GetLocation()));
	}

	BuildFullLayout();
	TileLayout.RemoveAll([&SurvivingTiles](FIntPoint const Tile) { return !SurvivingTiles.Contains(Tile); });
	RebuildInstances();
}

void AGeoHexBarrier::ResetLayout()
{
	TileLayout.Reset();
	RebuildInstances();
}

FTransform AGeoHexBarrier::GetTileTransform(FIntPoint const Tile) const
{
	FVector const TileLocation(TileSize * Sqrt3 * (Tile.X + 0.5f * (Tile.Y & 1)), TileSize * 1.5f * Tile.Y, 0.f);
	return FTransform(FRotator::ZeroRotator, TileLocation, FVector(TileSize / 100.f, TileSize / 100.f, 1.f));
}

FIntPoint AGeoHexBarrier::LocalToTile(FVector const& TileLocation) const
{
	int32 const Row = FMath::RoundToInt32(TileLocation.Y / (TileSize * 1.5f));
	return FIntPoint(FMath::RoundToInt32(TileLocation.X / (TileSize * Sqrt3) - 0.5f * (Row & 1)), Row);
}

void AGeoHexBarrier::Tick(float const DeltaSeconds)
{
	float const PreviousLerpAlpha = LerpAlpha;
	Super::Tick(DeltaSeconds);
	ApplyTileVisuals(/*bRemoving*/ LerpAlpha > PreviousLerpAlpha);
}

void AGeoHexBarrier::ApplyTileVisuals(bool const bRemoving)
{
	int32 const NumTiles = TileLayout.Num();
	int32 const HiddenCount = FMath::RoundToInt32(LerpAlpha * NumTiles);

	if (HiddenCount != AppliedHiddenCount)
	{
		int32 const FirstChanged = FMath::Min(HiddenCount, AppliedHiddenCount);
		int32 const LastChanged = FMath::Max(HiddenCount, AppliedHiddenCount);
		for (int32 Index = FirstChanged; Index < LastChanged; ++Index)
		{
			FTransform TileTransform = GetTileTransform(TileLayout[Index]);
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
			FTransform ShakeTransform = GetTileTransform(TileLayout[Index]);
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
			TileMeshComponent->UpdateInstanceTransform(Index, GetTileTransform(TileLayout[Index]),
													   /*bWorldSpace*/ false, /*bMarkRenderStateDirty*/ false,
													   /*bTeleport*/ true);
		}
		TileMeshComponent->MarkRenderStateDirty();
		bWasShaking = false;
	}
}
