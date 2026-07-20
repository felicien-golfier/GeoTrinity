// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Boss/GeoSpawnOnTileAbility.h"

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/GeoHexArena.h"
#include "Characters/Component/GeoDeployableManagerComponent.h"
#include "Tool/UGeoGameplayLibrary.h"

UGeoSpawnOnTileAbility::UGeoSpawnOnTileAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
}

void UGeoSpawnOnTileAbility::Fire(FGeoAbilityTargetData const& AbilityTargetData)
{
	Super::Fire(AbilityTargetData);

	AGeoHexArena* const Arena = AGeoHexArena::GetArenaOfBoss(StoredPayload.Owner);
	if (ensureMsgf(Arena, TEXT("UGeoSpawnOnTileAbility: %s is not a hex arena boss"), *GetNameSafe(StoredPayload.Owner))
		&& ensureMsgf(DeployableClass, TEXT("UGeoSpawnOnTileAbility: DeployableClass is not set")))
	{
		FRandomStream Stream(StoredPayload.Seed);
		for (FIntPoint const Tile : Arena->GetRandomAliveTiles(Stream, GetSpawnRing(*Arena), SpawnCount))
		{
			GeoASLib::FullySpawnDeployable(DeployableClass, StoredPayload, GetEffectDataArray(), DeployableParams,
										   FTransform(FVector(Arena->TileToWorld(Tile), ArbitraryCharacterZ)));
		}
	}

	EndAbility();
}

void UGeoSpawnOnTileAbility::OnGiveAbility(FGameplayAbilityActorInfo const* ActorInfo, FGameplayAbilitySpec const& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);
	// The boss's own deployable limit is meant for player deployables; the arena's tiles bound these instead.
	if (UGeoDeployableManagerComponent* const DeployableManager =
			StoredPayload.Instigator->GetComponentByClass<UGeoDeployableManagerComponent>())
	{
		DeployableManager->SetDeployableInfinitCount(DeployableClass);
	}
}

int32 UGeoSpawnOnTileAbility::GetSpawnRing(AGeoHexArena const& Arena) const
{
	if (!bSpawnOnHealthScaledRing)
	{
		return INDEX_NONE;
	}

	UGeoAbilitySystemComponent* const GeoAsc = GetGeoAbilitySystemComponentFromActorInfo();
	UGeoAttributeSetBase const* const AttributeSet =
		Cast<UGeoAttributeSetBase>(GeoAsc->GetAttributeSet(UGeoAttributeSetBase::StaticClass()));
	if (!ensureMsgf(AttributeSet, TEXT("UGeoSpawnOnTileAbility: boss ASC has no UGeoAttributeSetBase")))
	{
		return INDEX_NONE;
	}

	return FMath::RoundToInt32(AttributeSet->GetHealthRatio() * Arena.GetGridRadius());
}
