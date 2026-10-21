// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/Hazard/SafeSpot/SafeSpotSource.h"

#include "AbilitySystem/Abilities/Base/AbilityPayload.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Arena/GeoArena.h"
#include "Actor/Deployable/Pillar/GeoPillar.h"
#include "Characters/EnemyCharacter.h"
#include "Tool/Team.h"

TArray<FSafeSpot> GatherSafeSpots(UObject const* WorldContext, FAbilityPayload const& Payload,
								  TArray<ESafeSpotSource> const& Sources, ESafeSpotShadow const Shadow,
								  float const MaxDistance, float const Radius, TArray<FVector2D> const& FixedLocations)
{
	TArray<FSafeSpot> Spots;
	if (Sources.Contains(ESafeSpotSource::Pillars))
	{
		for (AGeoPillar* const Pillar : GeoASLib::GetInteractableActors<AGeoPillar>(
				 WorldContext, GeoASLib::GetTeamId(Payload.SourceOwner), TeamAttitudeMask::HostileOrNeutral, true,
				 Payload.Origin, MaxDistance))
		{
			Spots.Add({FVector2D(Pillar->GetActorLocation()), Pillar->GetSimpleCollisionRadius(), Shadow, Pillar});
		}
	}

	if (Sources.Contains(ESafeSpotSource::Adds))
	{
		AGeoArena const* const Arena = AGeoArena::GetArenaOfBoss(Payload.SourceAvatar);
		if (ensureMsgf(Arena, TEXT("%hs: %s has no arena to take add safe spots from"), __FUNCTION__,
					   *GetNameSafe(Payload.SourceAvatar)))
		{
			for (AEnemyCharacter const* const Add : Arena->GetLiveAdds())
			{
				Spots.Add({FVector2D(Add->GetActorLocation()), Radius, Shadow});
			}
		}
	}

	if (Sources.Contains(ESafeSpotSource::Fixed))
	{
		for (FVector2D const& Location : FixedLocations)
		{
			Spots.Add({Location, Radius, Shadow});
		}
	}

	return Spots;
}
