// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Boss/GeoSpawnOnDeathAbility.h"

#include "Actor/Arena/GeoArena.h"
#include "Characters/EnemyCharacter.h"
#include "GameClasses/Match/GeoGameState.h"

UGeoSpawnOnDeathAbility::UGeoSpawnOnDeathAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
}

void UGeoSpawnOnDeathAbility::ActivateAbility(FGameplayAbilitySpecHandle const Handle,
											  FGameplayAbilityActorInfo const* ActorInfo,
											  FGameplayAbilityActivationInfo const ActivationInfo,
											  FGameplayEventData const* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AGeoGameState* const GameState = GetWorld()->GetGameState<AGeoGameState>();
	if (ensureMsgf(GameState, TEXT("%s: no game state to watch deaths on"), *GetName()))
	{
		GameState->OnCharacterDied.AddUObject(this, &UGeoSpawnOnDeathAbility::OnCharacterDied);
	}
}

void UGeoSpawnOnDeathAbility::EndAbility(FGameplayAbilitySpecHandle const Handle,
										 FGameplayAbilityActorInfo const* ActorInfo,
										 FGameplayAbilityActivationInfo const ActivationInfo,
										 bool const bReplicateEndAbility, bool const bWasCancelled)
{
	AGeoGameState* const GameState = GetWorld() ? GetWorld()->GetGameState<AGeoGameState>() : nullptr;
	if (GameState)
	{
		GameState->OnCharacterDied.RemoveAll(this);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGeoSpawnOnDeathAbility::OnCharacterDied(AGeoCharacter& Character)
{
	AEnemyCharacter* const Owner = Cast<AEnemyCharacter>(GetAvatarActorFromActorInfo());
	AGeoArena* const Arena = Owner ? Owner->Arena.Get() : nullptr;
	if (Owner == &Character && ensureMsgf(Arena, TEXT("%s: %s died outside an arena"), *GetName(), *Owner->GetName())
		&& ensureMsgf(AllyClass, TEXT("%s: no AllyClass set"), *GetName()))
	{
		Arena->SpawnAdd(AllyClass, Owner->GetActorTransform(), &AllySetup);
	}
}
