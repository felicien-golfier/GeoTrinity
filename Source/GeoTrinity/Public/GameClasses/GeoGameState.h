// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Characters/PlayableCharacter.h"
#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "GameplayTagContainer.h"

#include "GeoGameState.generated.h"

class AEnemyCharacter;
class AGeoArenaBarrier;

USTRUCT(BlueprintType)
struct FEnemySpawnEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Enemy")
	TSubclassOf<AEnemyCharacter> EnemyClass;

	UPROPERTY(EditAnywhere, Category = "Enemy")
	FGameplayTag SpawnTag;
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemySpawned, AEnemyCharacter*, Enemy);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCommitFight);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMatchIsWaitingToStart);

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
	void SpawnEnemy(FEnemySpawnEntry const& Entry, bool bIsBoss);
	bool IsBoss(AActor const* Enemy) const;
	bool IsDummy(AActor const* Enemy) const;
	AEnemyCharacter* GetBossEnemy() const;

	UPROPERTY(BlueprintAssignable, Category = "Enemy")
	FOnEnemySpawned OnEnemySpawned;

	/** Level reference to the arena barrier actor. Set in the editor. */
	UPROPERTY(EditAnywhere, Category = "Fight")
	TObjectPtr<AGeoArenaBarrier> ArenaBarrier;

	/** Server-only. Called when a player dies during the fight. Decrements alive counter; triggers wipe reset when 0.
	 */
	void NotifyPlayerDiedInFight(APlayableCharacter* PlayableCharacter);

	/** Server-only. Called when the boss health reaches 0. Transitions to WaitingPostMatch. */
	UFUNCTION()
	void NotifyBossDefeated();

	void InitBoss(AEnemyCharacter* Boss);
	void SpawnEnemies();

	FCommitFight CommitFightDelegate;
	FMatchIsWaitingToStart MatchIsWaitingToStartDelegate;


private:
	UPROPERTY(EditAnywhere, Category = "Enemy")
	FEnemySpawnEntry BossToSpawn;

	UPROPERTY(EditAnywhere, Category = "Enemy")
	FEnemySpawnEntry DummyToSpawn;

	int32 PlayersAliveInFight = 0;

	FTimerHandle CommitFightTimer;

	void CommitFightStart();
	void TeleportPlayersTo(FGameplayTag LocationTag) const;
};
