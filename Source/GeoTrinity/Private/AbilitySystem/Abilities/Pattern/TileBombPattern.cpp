// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/TileBombPattern.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/GeoHexArena.h"
#include "Tool/Team.h"
#include "Tool/UGeoGameplayLibrary.h"

void UTileBombPattern::InitPattern(FAbilityPayload const& Payload, TInstancedStruct<FPatternData> const& PatternData)
{
	FTileBombPatternData const* const BombData = PatternData.GetPtr<FTileBombPatternData>();
	if (ensureMsgf(BombData,
				   TEXT("UTileBombPattern: PatternData is not an FTileBombPatternData — launch this pattern from "
						"UGeoTileBombAbility")))
	{
		BombCarrier = BombData->BombCarrier;
	}

	Super::InitPattern(Payload, PatternData);
}

FGameplayCueParameters UTileBombPattern::FillCueParam(FAbilityPayload const& Payload)
{
	FGameplayCueParameters CueParams = Super::FillCueParam(Payload);
	CueParams.RawMagnitude = BlastRadius;
	if (IsValid(BombCarrier))
	{
		CueParams.EffectCauser = BombCarrier;
		CueParams.Location = BombCarrier->GetActorLocation();
	}
	return CueParams;
}

void UTileBombPattern::StartPattern()
{
	Super::StartPattern();

	// The carrier may have died or left during the wind-up; the bomb simply fizzles with them.
	if (!GeoLib::IsServer(GetWorld()) || !IsValid(BombCarrier))
	{
		EndPattern();
		return;
	}

	FVector2D const BlastCenter(BombCarrier->GetActorLocation());
	UGeoAbilitySystemComponent* const SourceASC = GeoASLib::GetGeoAscFromActor(StoredPayload.Owner);
	if (ensureMsgf(SourceASC, TEXT("UTileBombPattern: Owner has no ASC")))
	{
		for (AActor* HitActor :
			 GeoASLib::GetInteractableActors(this, GeoASLib::GetTeamId(StoredPayload.Owner),
											 TeamAttitudeMask::HostileOrNeutral, true, BlastCenter, BlastRadius))
		{
			if (UGeoAbilitySystemComponent* const TargetASC = GeoASLib::GetGeoAscFromActor(HitActor))
			{
				GeoASLib::ApplyEffectFromEffectData(EffectDataArray, SourceASC, TargetASC, StoredPayload.AbilityLevel,
													StoredPayload.Seed, StoredPayload.AbilityTag);
			}

			if (!bPatternIsActive) // Cuz previous effect can kill the last char and so delete the boss.
			{
				return;
			}
		}
	}

	AGeoHexArena* const Arena = AGeoHexArena::GetArenaOfBoss(StoredPayload.Owner);
	if (ensureMsgf(Arena, TEXT("UTileBombPattern: %s is not a hex arena boss"), *GetNameSafe(StoredPayload.Owner)))
	{
		Arena->DestroyTilesInRadius(BlastCenter, BlastRadius);
	}

	EndPattern();
}
