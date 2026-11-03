// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/Hazard/Zone/SpawnAddPattern.h"

#include "Actor/Arena/GeoArena.h"
#include "Characters/EnemyCharacter.h"
#include "Tool/UGeoGameplayLibrary.h"

void USpawnAddPattern::OnCreate(FGameplayTag const AbilityTag, AActor& Owner)
{
	Super::OnCreate(AbilityTag, Owner);
	ensureMsgf(AddClass, TEXT("%hs: AddClass is not set on %s"), __FUNCTION__, *GetName());
}

void USpawnAddPattern::EndPattern(bool const bForceStop)
{
	if (IsPatternActive() && !bForceStop && GeoLib::IsServer(GetWorld()))
	{
		AGeoArena* const Arena = AGeoArena::GetArenaOfBoss(StoredPayload.SourceAvatar);
		if (ensureMsgf(Arena, TEXT("%s: the caster has no arena to summon into"), *GetName())
			&& ensureMsgf(AddClass, TEXT("%s: no AddClass set"), *GetName()))
		{
			for (FVector2D const& Location : GetTargetLocations())
			{
				Arena->SpawnAdd(AddClass, FTransform(FVector(Location, ArbitraryCharacterZ)), &AddSetup);
			}
		}
	}

	Super::EndPattern(bForceStop);
}
