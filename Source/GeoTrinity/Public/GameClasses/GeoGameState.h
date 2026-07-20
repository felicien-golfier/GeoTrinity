// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Characters/PlayableCharacter.h"
#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "GameplayTagContainer.h"

#include "GeoGameState.generated.h"

class AEnemyCharacter;
class AGeoArena;
class AGeoDeployableBase;
class UGeoDeployableManagerComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemySpawned, AEnemyCharacter*, Enemy);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FActiveArenaChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMatchStateChanged, FName, MatchState, FName, PreviousMatchState);

/**
 * Replicated game state for GeoTrinity. Answers two independent questions and nothing else: which arena the players
 * are in (ActiveArena) and whether a boss fight is running (MatchState). The encounter itself — target points, fight
 * commit, respawn policy — belongs to AGeoArena, so nothing here knows about a particular boss, barrier or room.
 */
UCLASS()
class GEOTRINITY_API AGeoGameState : public AGameState
{
	GENERATED_BODY()

public:
	/** Registers ActiveArena, which clients need to resolve the boss health bar and the fight camera bounds. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Calls StartBossFight() on the active arena's boss, which each arena spawned in its own BeginPlay. */
	virtual void HandleMatchHasStarted() override;
	/** Server. Revives all players currently in the world by calling ReviveLogic() on each pawn. */
	void RevivePlayers() const;
	/** Hides the boss health bar locally, ends the active arena's fight, and revives players (server). */
	void StopBossFight();

	/**
	 * Server. Stands the match down so the next boss aggro can start a new one. WaitingToStart is the only state
	 * AGameMode::StartMatch will act on — it early-outs on HasMatchStarted(), which is true in WaitingPostMatch too.
	 */
	void RequestWaitingToStart() const;

	/** Resets the active arena's enemies, claiming the DefaultArenaTag arena on the level's first call. */
	virtual void HandleMatchIsWaitingToStart() override;

	/** Calls StopBossFight() when the match transitions away from InProgress, on the server and on clients alike. */
	virtual void OnRep_MatchState() override;

	/** Server-only. Called when the boss health reaches 0. Captures the loot origin, transitions to WaitingPostMatch.
	 */
	UFUNCTION()
	void NotifyBossDefeated();
	/** Server-only. Starts the post-match loot shower: bursts of buff pickups erupting from the dead boss. */
	void Loot();

	/**
	 * Shows boss health bar locally, binds the defeat delegate, sends the aggro StateTree event and starts the active
	 * arena's fight. Server-side bindings only.
	 */
	void StartBossFight(AEnemyCharacter* Boss);

	UPROPERTY(BlueprintAssignable, Category = "Enemy")
	FOnEnemySpawned OnEnemySpawned;

	/** Server. Moves the players into Arena — a teleport to another room, or a boss aggro. Broadcasts
	 * OnActiveArenaChanged on every machine. */
	void SetActiveArena(AGeoArena* Arena);

	/** The level's arena carrying ArenaTag, or null when none does. */
	AGeoArena* FindArena(FGameplayTag ArenaTag) const;

	/** Arena the players are in — their checkpoint, their fight location, and the respawn policy their deaths run. */
	AGeoArena* GetActiveArena() const { return ActiveArena; }

	/** ArenaTag of the arena the players are in; DefaultArenaTag on a client until ActiveArena has replicated. */
	FGameplayTag GetActiveArenaTag() const;

	/** Broadcast whenever the players change arena, on server and clients alike. Drives the camera bounds. */
	FActiveArenaChanged OnActiveArenaChanged;

	/** Arena the players start the level in — the hub. Claimed as the first ActiveArena. */
	UPROPERTY(EditAnywhere, Category = "Fight")
	FGameplayTag DefaultArenaTag;

	/** Seconds from a fight starting to its commit. Shared by every arena; also the barrier's lerp duration. */
	UPROPERTY(EditAnywhere, Category = "Fight")
	float CommitFightTime = 3.f;

	/** Seconds between loot pickup bursts after the boss dies. */
	UPROPERTY(EditAnywhere, Category = "Loot")
	float LootSpawnInterval = 0.1f;
	/** Pickups spawned per burst. */
	UPROPERTY(EditAnywhere, Category = "Loot")
	int32 LootPickupsPerBurst = 1;
	/** Scatter radius around the dead boss the pickups are launched to. */
	UPROPERTY(EditAnywhere, Category = "Loot")
	float LootMaxRadius = 1500.f;

	FMatchStateChanged OnMatchStateChanged;

private:
	/**
	 * Arena the players are currently in — the hub at level start, then whichever room they teleported to or boss they
	 * aggroed. Never null on the server: it stays on the last arena entered, so a wipe sends everyone back to that
	 * arena's entrance rather than to the hub. Replicated because clients need it for the boss health bar and the
	 * camera bounds.
	 */
	UPROPERTY(ReplicatedUsing = OnRep_ActiveArena)
	TObjectPtr<AGeoArena> ActiveArena;

	UFUNCTION()
	void OnRep_ActiveArena();

	FTimerHandle LootTimer;
	FVector LootOrigin = FVector::ZeroVector;

	/** Managers granted an unlimited pickup slot for the loot shower; restored when the arena resets. */
	TArray<TWeakObjectPtr<UGeoDeployableManagerComponent>> LootBoostedManagers;
	TSubclassOf<AGeoDeployableBase> LootPickupClass;

	/** Spawns one burst of loot pickups from LootOrigin. Timer callback started by Loot(). */
	void SpawnLootBurst();

	/** Server. Stops the shower and gives back the pickup slots it borrowed. Runs when the next fight starts. */
	void StopLoot();
};
