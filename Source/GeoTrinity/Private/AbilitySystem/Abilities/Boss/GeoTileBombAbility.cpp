// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Boss/GeoTileBombAbility.h"

#include "AbilitySystem/Abilities/Pattern/TileBombPattern.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Characters/PlayableCharacter.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Tool/Team.h"

TInstancedStruct<FPatternData> UGeoTileBombAbility::CreatePatternData() const
{
	FTileBombPatternData BombData;

	TArray<APlayableCharacter*> const Candidates = GeoASLib::GetInteractableActors<APlayableCharacter>(
		this, GeoASLib::GetTeamId(StoredPayload.Owner), TeamAttitudeMask::HostileOrNeutral,
		/*bMustBeDamageable*/ true, StoredPayload.Origin, /*MaxDistance*/ 0.f);

	if (Candidates.IsEmpty())
	{
		UE_LOG(LogGeoASC, Warning, TEXT("UGeoTileBombAbility: no live player to carry the bomb"));
		return TInstancedStruct<FPatternData>::Make<FTileBombPatternData>(BombData);
	}

	FRandomStream Stream(StoredPayload.Seed);
	BombData.BombCarrier = Candidates[Stream.RandHelper(Candidates.Num())];

	return TInstancedStruct<FPatternData>::Make<FTileBombPatternData>(BombData);
}
