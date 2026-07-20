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
 * Replicated game state for GeoTrinity. Drives the boss fight lifecycle via UE's MatchState hooks: coordinates the
 * fight-commit timer, tracks how many players are still alive, and runs the post-match loot shower.
 * A level holds one AGeoArena per encounter; the match runs whichever one is active, so nothing here knows about a
 * particular boss, barrier or room.
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
	/** Destroys the boss, hides the boss health bar locally, opens the arena barrier, and revives players (server). */
	void StopBossFight();

	/** Resets the active arena's enemies (claiming the DefaultArenaTag arena on the first call), then teleports players
	 * back to that arena's entrance. */
	virtual void HandleMatchIsWaitingToStart() override;

	/** Calls StopBossFight(). */
	virtual void HandleMatchHasEnded() override;

	/** Client-side: calls StopBossFight() when the match transitions away from InProgress. */
	virtual void OnRep_MatchState() override;

	/**
	 * Server-only. Called when a player dies. Outside InProgress (e.g. during the fight-commit transition) it just
	 * revives that player; during the fight it re-checks for a full wipe.
	 */
	void NotifyPlayerDied(APlayableCharacter* PlayableCharacter);

	/** Server-only. Stops tracking a player who left the fight, then re-checks for a full wipe. */
	void NotifyPlayerLeft(APlayableCharacter* PlayableCharacter);

	/** Server-only. Called when the boss health reaches 0. Captures the loot origin, transitions to WaitingPostMatch.
	 */
	UFUNCTION()
	void NotifyBossDefeated();
	/** Server-only. Starts the post-match loot shower: bursts of buff pickups erupting from the dead boss. */
	void Loot();

	/**
	 * Shows boss health bar locally, binds the defeat delegate, sends the aggro StateTree event,
	 * starts the active arena's fight, and schedules the fight-commit timer. Server-side bindings only.
	 */
	void StartBossFight(AEnemyCharacter* Boss);

	UPROPERTY(BlueprintAssignable, Category = "Enemy")
	FOnEnemySpawned OnEnemySpawned;

	/** Server. Moves the players into Arena — a teleport to another room, or a boss aggro. Broadcasts
	 * OnActiveArenaChanged on every machine. */
	void SetActiveArena(AGeoArena* Arena);

	/** The level's arena carrying ArenaTag, or null when none does. */
	AGeoArena* FindArena(FGameplayTag ArenaTag) const;

	/** ArenaTag of the arena the players are in; DefaultArenaTag on a client until ActiveArena has replicated. */
	FGameplayTag GetActiveArenaTag() const;

	/** Broadcast whenever the players change arena, on server and clients alike. Drives the camera bounds. */
	FActiveArenaChanged OnActiveArenaChanged;

	/** Arena the players start the level in — the hub. Claimed as the first ActiveArena. */
	UPROPERTY(EditAnywhere, Category = "Fight")
	FGameplayTag DefaultArenaTag;

	UPROPERTY(EditAnywhere, Category = "Fight")
	float CommitFightTime = 3.f;
	UPROPERTY(EditAnywhere, Category = "Fight")
	float DeathTime = 3.f;
	/**
	 * Level reference to a trigger volume. On fight commit, players already overlapping this volume
	 * are left in place instead of being teleported to the arena's fight location. Set in the editor.
	 */
	UPROPERTY(EditAnywhere, Category = "Fight")
	FName FightZoneTagName = "FightZone";
	UPROPERTY(EditAnywhere, Category = "Fight")
	FName EntranceZoneTagName = "EntranceZone";

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
	FTimerHandle LootTimer;
	FVector LootOrigin = FVector::ZeroVector;

	/** Managers granted an unlimited pickup slot for the loot shower; restored when the arena resets. */
	TArray<TWeakObjectPtr<UGeoDeployableManagerComponent>> LootBoostedManagers;
	TSubclassOf<AGeoDeployableBase> LootPickupClass;

	/** Spawns one burst of loot pickups from LootOrigin. Timer callback started by Loot(). */
	void SpawnLootBurst();

	void CommitFightStart();
	/** Teleports players to the current arena's points carrying PurposeTag, skipping anyone inside the exempt zone. */
	void TeleportPlayersTo(FGameplayTag PurposeTag, FName const& ExemptZoneName = NAME_None) const;
	UFUNCTION()
	void RequestWaitingToStart() const;
};
