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
	/**
	 * Spawns enemies from `EnemiesToSpawn`, sends the aggro event to the boss's StateTree, counts connected
	 * players, and schedules the arena-lock timer (server). Shows the boss health bar locally.
	 */
	virtual void HandleMatchHasStarted() override;
	void RevivePlayers() const;
	void StopBossFight();

	/** Hides boss health bar locally. Opens the arena barrier and teleports players to the entrance (server). */
	virtual void HandleMatchIsWaitingToStart() override;

	/** Hides boss health bar locally. Opens the arena barrier (server). */
	virtual void HandleMatchHasEnded() override;

	virtual void OnRep_MatchState() override;

	void SpawnEnemy(FEnemySpawnEntry const& Entry);
	bool IsBoss(AActor const* Enemy) const;
	bool IsDummy(AActor const* Enemy) const;
	AEnemyCharacter* GetBossEnemy() const;
	AEnemyCharacter* GetDummyEnemy() const;

	/** Server-only. Called when a player dies during the fight. Decrements alive counter; triggers wipe reset when 0.
	 */
	void NotifyPlayerDiedInFight(APlayableCharacter* PlayableCharacter);

	/** Server-only. Called when the boss health reaches 0. Transitions to WaitingPostMatch. */
	UFUNCTION()
	void NotifyBossDefeated();

	void InitBossFight(AEnemyCharacter* Boss);
	AGeoArenaBarrier* GetArenaBarrier() const;
	void SpawnEnemies();

	UPROPERTY(EditAnywhere, Category = "Enemy")
	FEnemySpawnEntry BossToSpawn;
	UPROPERTY(EditAnywhere, Category = "Enemy")
	FEnemySpawnEntry DummyToSpawn;

	UPROPERTY(EditAnywhere, Category = "Fight")
	float CommitFightTime = 3.f;
	UPROPERTY(EditAnywhere, Category = "Fight")
	float DeathTime = 3.f;
	/**
	 * Level reference to a trigger volume. On fight commit, players already overlapping this volume
	 * are left in place instead of being teleported to the fight location. Set in the editor.
	 */
	UPROPERTY(EditAnywhere, Category = "Fight")
	FName FightZoneTagName = "FightZone";
	UPROPERTY(EditAnywhere, Category = "Fight")
	FName EntranceZoneTagName = "EntranceZone";

	FCommitFight CommitFightDelegate;
	FMatchIsWaitingToStart MatchIsWaitingToStartDelegate;

private:
	int32 PlayersAliveInFight = 0;

	FTimerHandle CommitFightTimer;

	void CommitFightStart() const;
	void TeleportPlayersTo(FGameplayTag LocationTag, FName const& ExemptZoneName = NAME_None) const;
	UFUNCTION()
	void RequestWaitingToStart() const;
};
