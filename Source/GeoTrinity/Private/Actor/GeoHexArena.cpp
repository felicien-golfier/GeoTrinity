// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/GeoHexArena.h"

#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "Actor/Deployable/GeoDeployableBase.h"
#include "Characters/PlayableCharacter.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Tool/UGeoGameplayLibrary.h"

namespace
{
	constexpr float Sqrt3 = 1.7320508f;

	FIntPoint RoundToNearestTile(float const Q, float const R)
	{
		int32 RoundedQ = FMath::RoundToInt32(Q);
		int32 RoundedR = FMath::RoundToInt32(R);
		int32 const RoundedS = FMath::RoundToInt32(-Q - R);
		float const DiffQ = FMath::Abs(RoundedQ - Q);
		float const DiffR = FMath::Abs(RoundedR - R);
		float const DiffS = FMath::Abs(RoundedS - (-Q - R));
		if (DiffQ > DiffR && DiffQ > DiffS)
		{
			RoundedQ = -RoundedR - RoundedS;
		}
		else if (DiffR > DiffS)
		{
			RoundedR = -RoundedQ - RoundedS;
		}
		return FIntPoint(RoundedQ, RoundedR);
	}
} // namespace

AGeoHexArena::AGeoHexArena()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	TileMeshComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("TileMeshComponent"));
	TileMeshComponent->SetupAttachment(RootComponent);
	TileMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AGeoHexArena::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGeoHexArena, TileStates);
}

void AGeoHexArena::OnConstruction(FTransform const& Transform)
{
	Super::OnConstruction(Transform);
	BuildGrid();
	TileMeshComponent->ClearInstances();
	for (FIntPoint const Tile : TileCoords)
	{
		TileMeshComponent->AddInstance(GetTileTransform(TileToLocal(Tile)));
	}
}

void AGeoHexArena::BeginPlay()
{
	Super::BeginPlay();
	BuildGrid();
	if (GeoLib::IsServer(this))
	{
		TileStates.Init(1, TileCoords.Num());
		AppliedTileStates = TileStates;
	}
	else
	{
		ApplyTileVisuals();
	}
}

void AGeoHexArena::CommitFight()
{
	Super::CommitFight();
	SetActorTickEnabled(true);
}

void AGeoHexArena::EndFight()
{
	Super::EndFight();
	SetActorTickEnabled(false);
	ResetAllTiles();
}

void AGeoHexArena::BuildGrid()
{
	TileCoords.Reset();
	CoordToIndex.Reset();
	for (int32 X = -GridRadius; X <= GridRadius; ++X)
	{
		for (int32 Y = FMath::Max(-GridRadius, -X - GridRadius); Y <= FMath::Min(GridRadius, -X + GridRadius); ++Y)
		{
			CoordToIndex.Add(FIntPoint(X, Y), TileCoords.Add(FIntPoint(X, Y)));
		}
	}
}

FVector AGeoHexArena::TileToLocal(FIntPoint const Tile) const
{
	return FVector(TileSize * Sqrt3 * (Tile.X + 0.5f * Tile.Y), TileSize * 1.5f * Tile.Y, 0.f);
}

FTransform AGeoHexArena::GetTileTransform(FVector const& TileLocation) const
{
	return FTransform(FRotator::ZeroRotator, TileLocation, FVector(TileSize / 100.f, TileSize / 100.f, 1.f));
}

FVector2D AGeoHexArena::TileToWorld(FIntPoint const Tile) const
{
	return FVector2D(GetActorTransform().TransformPosition(TileToLocal(Tile)));
}

bool AGeoHexArena::GetTileUnderLocation(FVector2D const WorldLocation, FIntPoint& OutTile) const
{
	FVector const Local = GetActorTransform().InverseTransformPosition(FVector(WorldLocation, 0.f));
	float const Q = (Local.X / Sqrt3 - Local.Y / 3.f) / TileSize;
	float const R = (2.f / 3.f) * Local.Y / TileSize;
	OutTile = RoundToNearestTile(Q, R);
	return CoordToIndex.Contains(OutTile);
}

bool AGeoHexArena::IsTileAlive(FIntPoint const Tile) const
{
	int32 const* Index = CoordToIndex.Find(Tile);
	return Index && TileStates.IsValidIndex(*Index) && TileStates[*Index] != 0;
}

bool AGeoHexArena::IsOverAliveTile(FVector2D const WorldLocation) const
{
	FIntPoint Tile;
	return GetTileUnderLocation(WorldLocation, Tile) && IsTileAlive(Tile);
}

AGeoHexArena* AGeoHexArena::GetArenaOfBoss(AActor const* Boss)
{
	return Cast<AGeoHexArena>(AGeoArena::GetArenaOfBoss(Boss));
}

int32 AGeoHexArena::GetTileRing(FIntPoint const Tile)
{
	return (FMath::Abs(Tile.X) + FMath::Abs(Tile.Y) + FMath::Abs(Tile.X + Tile.Y)) / 2;
}

TArray<FIntPoint> AGeoHexArena::GetRandomAliveTiles(FRandomStream& Stream, int32 const Ring, int32 const Count) const
{
	TArray<FIntPoint> Candidates;
	for (FIntPoint const Tile : TileCoords)
	{
		if (IsTileAlive(Tile) && (Ring < 0 || GetTileRing(Tile) == Ring))
		{
			Candidates.Add(Tile);
		}
	}
	if (Candidates.IsEmpty() && Ring >= 0)
	{
		return GetRandomAliveTiles(Stream, -1, Count);
	}

	TArray<FIntPoint> PickedTiles;
	while (PickedTiles.Num() < Count && !Candidates.IsEmpty())
	{
		int32 const CandidateIndex = Stream.RandHelper(Candidates.Num());
		PickedTiles.Add(Candidates[CandidateIndex]);
		Candidates.RemoveAtSwap(CandidateIndex);
	}
	return PickedTiles;
}

bool AGeoHexArena::GetLastAliveTileAlongRay(FVector2D const Origin, FVector2D const Direction, float const MaxRange,
										   FIntPoint& OutTile) const
{
	FVector2D const Forward = Direction.GetSafeNormal();
	// Half a tile keeps the walk from stepping over a tile the ray only clips.
	float const StepSize = TileSize * 0.5f;
	// Past the far edge of the platform there is nothing left to cross, however long the ray is.
	float const WalkDistance = FMath::Min(
		MaxRange, FVector2D::Distance(Origin, FVector2D(GetActorLocation())) + GridRadius * TileSize * Sqrt3);

	bool bFoundTile = false;
	for (float Distance = 0.f; Distance <= WalkDistance; Distance += StepSize)
	{
		FIntPoint Tile;
		if (GetTileUnderLocation(Origin + Forward * Distance, Tile) && IsTileAlive(Tile))
		{
			OutTile = Tile;
			bFoundTile = true;
		}
	}
	return bFoundTile;
}

void AGeoHexArena::DestroyTiles(TConstArrayView<FIntPoint> const Tiles)
{
	if (!ensureMsgf(GeoLib::IsServer(this), TEXT("DestroyTiles is server-only")))
	{
		return;
	}
	for (FIntPoint const Tile : Tiles)
	{
		if (int32 const* Index = CoordToIndex.Find(Tile))
		{
			TileStates[*Index] = 0;
		}
	}
	ApplyTileVisuals();
}

void AGeoHexArena::DestroyTilesInRadius(FVector2D const Center, float const Radius)
{
	if (!ensureMsgf(GeoLib::IsServer(this), TEXT("DestroyTilesInRadius is server-only")))
	{
		return;
	}
	for (int32 Index = 0; Index < TileCoords.Num(); ++Index)
	{
		if (FVector2D::DistSquared(TileToWorld(TileCoords[Index]), Center) <= FMath::Square(Radius))
		{
			TileStates[Index] = 0;
		}
	}
	ApplyTileVisuals();
}

void AGeoHexArena::ResetAllTiles()
{
	if (!ensureMsgf(GeoLib::IsServer(this), TEXT("ResetAllTiles is server-only")))
	{
		return;
	}
	TileStates.Init(1, TileCoords.Num());
	ApplyTileVisuals();
}

void AGeoHexArena::OnRep_TileStates()
{
	ApplyTileVisuals();
}

void AGeoHexArena::ApplyTileVisuals()
{
	if (TileCoords.IsEmpty())
	{
		BuildGrid();
	}
	if (AppliedTileStates.Num() != TileStates.Num())
	{
		AppliedTileStates.Init(1, TileStates.Num());
	}

	bool bAnyChange = false;
	for (int32 Index = 0; Index < TileStates.Num(); ++Index)
	{
		if (TileStates[Index] == AppliedTileStates[Index])
		{
			continue;
		}
		FTransform TileTransform(GetTileTransform(TileToLocal(TileCoords[Index])));
		if (TileStates[Index] == 0)
		{
			TileTransform.SetScale3D(FVector::ZeroVector);
		}
		TileMeshComponent->UpdateInstanceTransform(Index, TileTransform, /*bWorldSpace*/ false,
												   /*bMarkRenderStateDirty*/ false, /*bTeleport*/ true);
		AppliedTileStates[Index] = TileStates[Index];
		bAnyChange = true;
	}
	if (bAnyChange)
	{
		TileMeshComponent->MarkRenderStateDirty();
	}
}

void AGeoHexArena::Tick(float const DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	FVector2D const Center(GetActorLocation());
	for (AActor* Actor : GeoASLib::GetInteractableActors(this, /*bMustBeDamageable*/ false, Center, FallCheckRadius))
	{
		if (IsOverAliveTile(FVector2D(Actor->GetActorLocation())))
		{
			continue;
		}

		if (APlayableCharacter* Player = Cast<APlayableCharacter>(Actor))
		{
			if (!Player->IsDead() && !Player->GetCharacterMovement()->HasRootMotionSources())
			{
				KillFallenPlayer(*Player);
			}
		}
		else if (AGeoDeployableBase* Deployable = Cast<AGeoDeployableBase>(Actor))
		{
			Deployable->Recall();
		}
	}
}

void AGeoHexArena::KillFallenPlayer(APlayableCharacter& Player) const
{
	Player.Death();
	TArray<AActor*> const RespawnPoints =
		GeoLib::GetTargetPoints(this, FGeoGameplayTags::Get().TargetPoint_FallRespawn, ArenaTag);
	if (!ensureMsgf(!RespawnPoints.IsEmpty(), TEXT("%s has no TargetPoint.FallRespawn point tagged %s"), *GetName(),
					*ArenaTag.ToString()))
	{
		return;
	}
	Player.SetActorLocation(RespawnPoints[0]->GetActorLocation());
}
