// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/Action/GeoSpawnEnemiesAction.h"

#include "Characters/EnemyCharacter.h"
#include "Encounter/GeoEncounter.h"
#include "Engine/World.h"
#include "GameClasses/Match/GeoGameState.h"
#include "GeoTrinity/GeoTrinity.h"
#include "TimerManager.h"
#include "Tool/UGeoGameplayLibrary.h"

void UGeoSpawnEnemiesAction::Execute_Implementation(FGeoEncounterPayload const& /*Payload*/)
{
	ensureMsgf(EnemyClass, TEXT("%s: no EnemyClass set"), *GetName());
	OnCancel_Implementation();
	GetEncounter()->HoldUntilStepLeft(*this);

	TArray<AActor*> const SpawnPoints = GeoLib::GetTargetPoints(this, PurposeTag, ArenaTag);
	if (SpawnPoints.IsEmpty() || !EnemyClass)
	{
		UE_LOG(LogGeoEncounter, Warning, TEXT("%s: no %s point tagged %s to spawn %s on"), *GetName(),
			   *PurposeTag.ToString(), *ArenaTag.ToString(), *GetNameSafe(EnemyClass));
	}
	else
	{
		for (int32 Index = 0; Index < Count; ++Index)
		{
			SpawnAt(SpawnPoints[Index % SpawnPoints.Num()]->GetActorLocation());
		}

		AGeoGameState* const GameState = GetWorld()->GetGameState<AGeoGameState>();
		if (RespawnDelay > 0.f && GameState)
		{
			GameState->OnCharacterDied.AddUObject(this, &UGeoSpawnEnemiesAction::OnCharacterDied);
		}
	}
}

void UGeoSpawnEnemiesAction::OnCancel_Implementation()
{
	if (AGeoGameState* const GameState = GetWorld()->GetGameState<AGeoGameState>())
	{
		GameState->OnCharacterDied.RemoveAll(this);
	}

	for (FTimerHandle& Timer : RespawnTimers)
	{
		GetWorld()->GetTimerManager().ClearTimer(Timer);
	}

	RespawnTimers.Reset();

	TArray<TObjectPtr<AEnemyCharacter>> const Living = MoveTemp(Spawned);
	if (bKillOnStepExit)
	{
		for (AEnemyCharacter* const Enemy : Living)
		{
			if (IsValid(Enemy))
			{
				Enemy->Death();
			}
		}
	}
}

void UGeoSpawnEnemiesAction::SpawnAt(FVector const Location)
{
	if (AEnemyCharacter* const Enemy =
			AEnemyCharacter::SpawnEnemy(*GetWorld(), EnemyClass, Location, GetEncounter(), nullptr, &Setup))
	{
		Spawned.Add(Enemy);
	}
}

void UGeoSpawnEnemiesAction::OnCharacterDied(AGeoCharacter& Character)
{
	if (Spawned.RemoveSingle(Cast<AEnemyCharacter>(&Character)) > 0)
	{
		FTimerHandle& Timer = RespawnTimers.AddDefaulted_GetRef();
		GetWorld()->GetTimerManager().SetTimer(
			Timer, FTimerDelegate::CreateUObject(this, &UGeoSpawnEnemiesAction::SpawnAt, Character.GetActorLocation()),
			RespawnDelay, false);
	}
}
