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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMatchStateChanged, FName, MatchState, FName, PreviousMatchState);

/**
 * Replicated game state for GeoTrinity. Drives the boss fight lifecycle via
 * UE's MatchState hooks: spawns enemies, manages the arena barrier, coordinates
 * the fight-commit timer, and tracks how many players are still alive.
 */
UCLASS()
class GEOTRINITY_API AGeoGameState : public AGameState
{
	GENERATED_BODY()

public:
	/**
	 * Calls InitBossFight() on the existing boss enemy. Enemies are spawned during HandleMatchIsWaitingToStart;
	 * this hook assumes the boss is already in the world when the match begins.
	 */
	virtual void HandleMatchHasStarted() override;
	/** Server. Revives all players currently in the world by calling ReviveLogic() on each pawn. */
	void RevivePlayers() const;
	/** Destroys the boss, hides the boss health bar locally, opens the arena barrier, and revives players (server). */
	void StopBossFight();

	/** Teleports players to the entrance zone and spawns enemies (if not already alive). */
	virtual void HandleMatchIsWaitingToStart() override;

	/** Calls StopBossFight(). */
	virtual void HandleMatchHasEnded() override;

	/** Client-side: calls StopBossFight() when the match transitions away from InProgress. */
	virtual void OnRep_MatchState() override;

	/** Spawns a single enemy from Entry at the tagged spawn point. Server-only. */
	void SpawnEnemy(FEnemySpawnEntry const& Entry);
	/** Returns true if Enemy's class matches BossToSpawn. */
	bool IsBoss(AActor const* Enemy) const;
	/** Returns true if Enemy's class matches DummyToSpawn. */
	bool IsDummy(AActor const* Enemy) const;
	/** Finds and returns the live boss enemy actor in the world, or nullptr if none exists. */
	AEnemyCharacter* GetBossEnemy() const;
	/** Finds and returns the live dummy enemy actor in the world, or nullptr if none exists. */
	AEnemyCharacter* GetDummyEnemy() const;

	/**
	 * Server-only. Called when a player dies. Outside InProgress (e.g. during the fight-commit transition) it just
	 * revives that player; during the fight it re-checks for a full wipe.
	 */
	void NotifyPlayerDied(APlayableCharacter* PlayableCharacter);

	/** Server-only. Stops tracking a player who left the fight, then re-checks for a full wipe. */
	void NotifyPlayerLeft(APlayableCharacter* PlayableCharacter);

	/** Server-only. Called when the boss health reaches 0. Transitions to WaitingPostMatch. */
	UFUNCTION()
	void NotifyBossDefeated();

	/**
	 * Shows boss health bar locally, binds the defeat delegate, sends the aggro StateTree event,
	 * closes the arena barrier, and schedules the fight-commit timer. Server-side bindings only.
	 */
	void InitBossFight(AEnemyCharacter* Boss);
	/** Finds and returns the AGeoArenaBarrier in the level, or nullptr if none exists. */
	AGeoArenaBarrier* GetArenaBarrier() const;
	/** Spawns both boss and dummy enemies on the server. No-op on clients or if already spawned. */
	void SpawnEnemies();

	UPROPERTY(BlueprintAssignable, Category = "Enemy")
	FOnEnemySpawned OnEnemySpawned;
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
	FMatchStateChanged OnMatchStateChanged;

private:
	/** Players being tracked for the current fight. Their individual life is checked to detect a full wipe. */
	UPROPERTY()
	TArray<TObjectPtr<APlayableCharacter>> PlayersInFight;

	/** Returns true when every tracked player is gone (dead or no longer valid). */
	bool AreAllPlayersDead() const;

	/**
	 * Re-checks whether every tracked player is dead and, if so, resets the boss and schedules the transition
	 * back to WaitingToStart.
	 */
	void HandlePotentialWipe();

	FTimerHandle CommitFightTimer;

	void CommitFightStart() const;
	void TeleportPlayersTo(FGameplayTag LocationTag, FName const& ExemptZoneName = NAME_None) const;
	UFUNCTION()
	void RequestWaitingToStart() const;
};
