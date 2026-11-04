// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Boss/GeoSpawnEnemyCharacterAbility.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "Actor/Arena/GeoArena.h"
#include "Characters/EnemyCharacter.h"
#include "Tool/UGeoGameplayLibrary.h"

UGeoSpawnEnemyCharacterAbility::UGeoSpawnEnemyCharacterAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
	bUseGeneralChargeTimeForFireDelay = false;
	FireDelay = 0.f;
}

void UGeoSpawnEnemyCharacterAbility::OnGiveAbility(FGameplayAbilityActorInfo const* ActorInfo,
												   FGameplayAbilitySpec const& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	UGeoAbilitySystemComponent* const ASC = Cast<UGeoAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get());
	if (bSpawnOnOwnerDying && ensureMsgf(ASC, TEXT("%s: no ASC at OnGiveAbility"), *GetName()))
	{
		OwnSpecHandle = Spec.Handle;
		ASC->GenericGameplayEventCallbacks.FindOrAdd(FGeoGameplayTags::Get().Event_Character_Dying)
			.AddUObject(this, &UGeoSpawnEnemyCharacterAbility::OnOwnerDying);
	}
}

void UGeoSpawnEnemyCharacterAbility::OnOwnerDying(FGameplayEventData const* /*EventData*/)
{
	GetAbilitySystemComponentFromActorInfo()->TryActivateAbility(OwnSpecHandle);
}

void UGeoSpawnEnemyCharacterAbility::Fire(FGeoAbilityTargetData const& AbilityTargetData)
{
	Super::Fire(AbilityTargetData);

	AGeoArena* const Arena = AGeoArena::GetArenaOfBoss(StoredPayload.SourceAvatar);
	if (ensureMsgf(Arena, TEXT("%s: the caster has no arena to spawn into"), *GetName())
		&& ensureMsgf(EnemyClass, TEXT("%s: no EnemyClass set"), *GetName()))
	{
		for (FVector2D const& Location : GeoASLib::GetTargetLocations(Target, MakeTargetContext()))
		{
			Arena->SpawnAdd(EnemyClass, FTransform(FVector(Location, ArbitraryCharacterZ)), &Setup);
		}
	}

	EndAbility();
}
