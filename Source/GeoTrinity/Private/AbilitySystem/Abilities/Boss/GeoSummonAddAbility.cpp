// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Boss/GeoSummonAddAbility.h"

#include "Actor/Arena/GeoArena.h"
#include "Characters/EnemyCharacter.h"
#include "Characters/PlayableCharacter.h"
#include "Tool/UGeoGameplayLibrary.h"

FVector2D UGeoSummonAddAbility::GetFireOrigin2D(AActor* Instigator, UGeoAbilitySystemComponent* /*SourceASC*/,
												int const Seed) const
{
	TArray<APlayableCharacter*> const Players = GeoLib::GetAlivePlayers(Instigator);
	if (Players.IsEmpty())
	{
		return FVector2D(Instigator->GetActorLocation());
	}

	FRandomStream Stream(Seed);
	APlayableCharacter const* const Player = Players[Stream.RandHelper(Players.Num())];
	FVector2D const Offset = FVector2D(Stream.GetUnitVector()).GetSafeNormal() * Stream.FRandRange(0.f, SpotRadius);
	return FVector2D(Player->GetActorLocation()) + Offset;
}

void UGeoSummonAddAbility::OnPatternEnd()
{
	AGeoArena* const Arena = AGeoArena::GetArenaOfBoss(StoredPayload.SourceAvatar);
	if (ensureMsgf(Arena, TEXT("%s: the caster has no arena to summon into"), *GetName())
		&& ensureMsgf(AddClass, TEXT("%s: no AddClass set"), *GetName()))
	{
		Arena->SpawnAdd(AddClass, FTransform(FVector(StoredPayload.Origin, ArbitraryCharacterZ)));
	}

	Super::OnPatternEnd();
}
