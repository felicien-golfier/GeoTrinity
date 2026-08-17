// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/TileCarveRayPattern.h"

#include "Actor/Arena/GeoHexArena.h"
#include "Tool/UGeoGameplayLibrary.h"

void UTileCarveRayPattern::TickPattern(float const ServerTime, float const SpentTime)
{
	if (bDestroyLastTileHit && GeoLib::IsServer(GetWorld()))
	{
		FIntPoint LastTile;
		if (AGeoHexArena* const Arena = FindLastTileHit(SpentTime, LastTile))
		{
			Arena->HighlightTile(StoredPayload.Instigator, LastTile);
		}
	}

	Super::TickPattern(ServerTime, SpentTime);
}

void UTileCarveRayPattern::EndPattern(bool const bForceStop)
{
	if (IsPatternActive() && !bForceStop && bDestroyLastTileHit && GeoLib::IsServer(GetWorld()))
	{
		// Yaw at SpentTime 0: the tile that dies is the one the beam locked onto when it went live.
		FIntPoint LastTile;
		if (AGeoHexArena* const Arena = FindLastTileHit(0.f, LastTile))
		{
			Arena->DestroyTiles({LastTile});
		}
	}

	Super::EndPattern(bForceStop);
}

AGeoHexArena* UTileCarveRayPattern::FindLastTileHit(float const SpentTime, FIntPoint& OutTile) const
{
	AGeoHexArena* const Arena = AGeoHexArena::GetArenaOfBoss(StoredPayload.Owner);
	FVector2D const Forward(FRotator(0.f, GetBeamYaw(SpentTime), 0.f).Vector());
	if (!ensureMsgf(Arena, TEXT("UTileCarveRayPattern: %s is not a hex arena boss"), *GetNameSafe(StoredPayload.Owner))
		|| !Arena->GetLastAliveTileAlongRay(FVector2D(GetBeamOrigin()), Forward, OutTile))
	{
		return nullptr;
	}

	return Arena;
}
