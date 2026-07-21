// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "GameplayTagContainer.h"

#include "GeoGameState.generated.h"

class AEnemyCharacter;
class APlayableCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemySpawned, AEnemyCharacter*, Enemy);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMatchStateChanged, FName, MatchState, FName, PreviousMatchState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWipe, float, DeathTime);

/**
 * Replicated game state for GeoTrinity. It runs the match lifecycle (MatchState: WaitingToStart until a boss is
 * aggroed, InProgress while a fight runs) and the player death policy, and nothing else — it holds no pointer to any
 * arena, boss, barrier or room. The encounter reacts to the lifecycle on its own: see AGeoArena, which subscribes to
 * OnMatchStateChanged. The one thing a death needs from the encounter is where to come back, and that arrives as a
 * plain tag (CheckpointTag) the aggroed arena registers.
 */
UCLASS()
class GEOTRINITY_API AGeoGameState : public AGameState
{
	GENERATED_BODY()

public:
	/** Server. Snapshots the players alive as the fight begins into FightPlayers, the set a wipe is measured against. */
	virtual void HandleMatchHasStarted() override;

	/** Server. Revives every player pawn currently in the world (no-op on a living one, so overlapping calls are free). */
	void RevivePlayers() const;

	/**
	 * Server. A player just went down (death or disconnect). Out of a fight the player is revived on the spot; during a
	 * fight (match in progress) they stay down, and once every player who was alive when the fight began is down, the
	 * whole group respawns at the checkpoint after DeathTime. This is the single death policy — no arena is consulted.
	 */
	void NotifyPlayerDied(APlayableCharacter& Player);

	/** Server. The arena owning the current fight registers where a wipe returns the group (its own Arena.* tag, or the
	 *  next one to advance the checkpoint). Read only during a respawn, so it never needs to replicate. */
	void SetCheckpointTag(FGameplayTag Tag) { CheckpointTag = Tag; }

	/**
	 * Server. Stands the match down so the next boss aggro can start a new one. WaitingToStart is the only state
	 * AGameMode::StartMatch will act on — it early-outs on HasMatchStarted(), which is true in WaitingPostMatch too.
	 */
	void RequestWaitingToStart() const;

	/** Server-side revives on leaving InProgress (mid-fight casualties on a victory); broadcasts OnMatchStateChanged. */
	virtual void OnRep_MatchState() override;

	UPROPERTY(BlueprintAssignable, Category = "Enemy")
	FOnEnemySpawned OnEnemySpawned;

	/** Seconds from a fight starting to its commit. Shared by every arena; also the barrier's closing lerp duration.
	 *  A plain timing constant, not an arena reference, so it stays here where the arena and the barrier both read it. */
	UPROPERTY(EditAnywhere, Category = "Fight")
	float CommitFightTime = 3.f;

	/** Seconds the downed group stays on the ground after a wipe before it respawns at the checkpoint; also the
	 *  barrier's opening lerp duration, so it finishes opening right as the group comes back. */
	UPROPERTY(EditAnywhere, Category = "Fight")
	float DeathTime = 3.f;

	/** Broadcast on every match state transition, on server and clients alike. AGeoArena subscribes to run its fight. */
	FMatchStateChanged OnMatchStateChanged;

	/** Server. Broadcast the moment every fight player is down — DeathTime seconds before the group respawns. */
	FOnWipe OnWipe;

private:
	/** Arena.* tag a wipe respawns at; set by the arena that owns the current fight. */
	FGameplayTag CheckpointTag;

	/** Players alive when the current fight began. A wipe is "all of these are down", so late joiners — who are not
	 *  in this snapshot — can neither block a wipe nor trigger one. */
	TArray<TWeakObjectPtr<APlayableCharacter>> FightPlayers;

	FTimerHandle RespawnTimer;

	/** Returns true when no player from the fight-start snapshot is still standing. */
	bool AreFightPlayersDead() const;

	/** Server. Teleports the group to the checkpoint, revives everyone, and stands the match down. Wipe timer callback.
	 */
	void RespawnGroup();
};
