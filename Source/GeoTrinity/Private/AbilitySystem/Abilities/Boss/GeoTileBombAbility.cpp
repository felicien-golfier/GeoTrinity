// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Boss/GeoTileBombAbility.h"

#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Characters/PlayableCharacter.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Tool/Team.h"

UGeoTileBombAbility::UGeoTileBombAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
}

void UGeoTileBombAbility::Fire(FGeoAbilityTargetData const& AbilityTargetData)
{
	Super::Fire(AbilityTargetData);

	TArray<APlayableCharacter*> const Candidates = GeoASLib::GetInteractableActors<APlayableCharacter>(
		this, GeoASLib::GetTeamId(StoredPayload.Owner), TeamAttitudeMask::HostileOrNeutral,
		/*bMustBeDamageable*/ true, StoredPayload.Origin, /*MaxDistance*/ 0.f);

	if (Candidates.IsEmpty())
	{
		UE_LOG(LogGeoASC, Warning, TEXT("UGeoTileBombAbility: no live player to carry the bomb"));
		EndAbility();
		return;
	}

	FRandomStream Stream(StoredPayload.Seed);
	APlayableCharacter* const Carrier = Candidates[Stream.RandHelper(Candidates.Num())];

	if (AGeoDeployableBase* const Bomb = GeoASLib::FullySpawnDeployable(
			BombClass, StoredPayload, GetEffectDataArray(), BombParams, Carrier->GetActorTransform()))
	{
		Bomb->AttachToActor(Carrier, FAttachmentTransformRules::KeepWorldTransform);
	}

	EndAbility();
}
