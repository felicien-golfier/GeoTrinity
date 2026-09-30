// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/TileCarveRayPattern.h"

#include "Actor/Arena/GeoHexArena.h"
#include "Tool/UGeoGameplayLibrary.h"

void UTileCarveRayPattern::TickPattern(float const ServerTime, float const SpentTime)
{
	if (bDestroyLastTileHit && SpentTime < BeamDuration && GeoLib::IsServer(GetWorld()))
	{
		FIntPoint LastTile;
		if (AGeoHexArena* const Arena = FindLastTileHit(SpentTime, LastTile))
		{
			Arena->HighlightTile(StoredPayload.SourceAvatar, LastTile);
			HighlightedTile = LastTile;
		}
	}

	Super::TickPattern(ServerTime, SpentTime);
}

void UTileCarveRayPattern::EndPattern(bool bForceStop)
{
	Super::EndPattern(bForceStop);

	if (bDestroyLastTileHit && GeoLib::IsServer(GetWorld()))
	{
		if (AGeoHexArena* const Arena = AGeoHexArena::GetArenaOfBoss(StoredPayload.SourceAvatar))
		{
			Arena->ClearHighlight(StoredPayload.SourceAvatar);
			if (!bForceStop && HighlightedTile.IsSet())
			{
				Arena->DestroyTiles({HighlightedTile.GetValue()});
			}
		}

		HighlightedTile.Reset();
	}
}

AGeoHexArena* UTileCarveRayPattern::FindLastTileHit(float const SpentTime, FIntPoint& OutTile) const
{
	AGeoHexArena* const Arena = AGeoHexArena::GetArenaOfBoss(StoredPayload.SourceAvatar);
	float const ServerTime = GeoLib::GetServerTime(GetWorld(), true);
	FVector2D const Forward(FRotator(0.f, GetBeamYaw(SpentTime, ServerTime), 0.f).Vector());
	if (!ensureMsgf(Arena, TEXT("UTileCarveRayPattern: %s is not a hex arena boss"),
					*GetNameSafe(StoredPayload.SourceAvatar))
		|| !Arena->GetLastAliveTileAlongRay(FVector2D(GetBeamOrigin(ServerTime)), Forward, OutTile))
	{
		return nullptr;
	}

	return Arena;
}
