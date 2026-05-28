// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "GameplayTagContainer.h"

#include "GeoGameState.generated.h"

class AEnemyCharacter;
class AGeoArenaBarrier;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemySpawned, AEnemyCharacter*, Enemy);

/**
 * Replicated game state for GeoTrinity. Tracks spawned enemies and broadcasts
 * delegate events so HUD and other systems can react to enemy appearance.
 * Also drives the boss fight lifecycle via UE's built-in MatchState hooks.
 */
UCLASS()
class GEOTRINITY_API AGeoGameState : public AGameState
{
	GENERATED_BODY()

public:
	virtual void HandleMatchHasStarted() override;
	virtual void HandleMatchIsWaitingToStart() override;
	virtual void HandleMatchHasEnded() override;

	UPROPERTY(BlueprintAssignable, Category = "Enemy")
	FOnEnemySpawned OnEnemySpawned;

	/** Returns the first spawned enemy, or nullptr if none have been spawned yet. */
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	AEnemyCharacter* GetFirstEnemy() const { return SpawnedEnemies.Num() > 0 ? SpawnedEnemies[0] : nullptr; }

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Enemy")
	TArray<TObjectPtr<AEnemyCharacter>> SpawnedEnemies;

	/** Level reference to the arena barrier actor. Set in the editor. */
	UPROPERTY(EditAnywhere, Category = "Fight")
	TObjectPtr<AGeoArenaBarrier> ArenaBarrier;

	/** Server-only. Called when a player dies during the fight. Decrements alive counter; triggers wipe reset when 0. */
	void NotifyPlayerDiedInFight();

	/** Server-only. Called when the boss health reaches 0. Transitions to WaitingPostMatch. */
	UFUNCTION()
	void NotifyBossDefeated();

private:
	UPROPERTY(EditAnywhere, Category = "Enemy")
	TArray<TSubclassOf<AEnemyCharacter>> EnemiesToSpawn;

	int32 PlayersAliveInFight = 0;

	FTimerHandle CommitFightTimer;

	void CommitFightStart();
	void TeleportPlayersTo(FGameplayTag LocationTag);
};
