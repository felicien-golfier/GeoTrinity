// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "GameplayTagContainer.h"
#include "Tool/GeoDifficulty.h"

#include "GeoGameState.generated.h"

class AEnemyCharacter;
class APlayableCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemySpawned, AEnemyCharacter*, Enemy);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMatchStateChanged, FName, MatchState, FName, PreviousMatchState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWipe, float, DeathTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDifficultyChanged);

/**
 * Replicated game state for GeoTrinity. It runs the match lifecycle (MatchState: WaitingToStart until a boss is
 * aggroed, InProgress while a fight runs) and the player death policy, and nothing else — it holds no pointer to any
 * arena, boss, barrier or room. The encounter reacts to the lifecycle on its own: see AGeoArena, which subscribes to
 * OnMatchStateChanged. The one thing a death needs from the encounter is where to come back, and that arrives as a
 * plain tag (CurrentArenaTag) written by whichever AGeoArenaVolume the players walked into, or by the arena whose
 * fight is live.
 */
UCLASS()
class GEOTRINITY_API AGeoGameState : public AGameState
{
	GENERATED_BODY()

public:
	/** Registers Difficulty, so a floor pad lights up for the live tuning on every machine. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Server. Snapshots the players alive as the fight begins into FightPlayers, the set a wipe is measured against. */
	virtual void HandleMatchHasStarted() override;

	/**
	 * Server. Retunes the run and broadcasts, which is all a retune is: every arena respawns its boss off
	 * OnDifficultyChanged, and the floor pads repaint. Refused while a fight is in progress, since the boss it would
	 * delete is the one being fought.
	 */
	void SetDifficulty(EGeoDifficulty NewDifficulty);
	/** The tuning every boss runs. Its bit value is the level AEnemyCharacter::InitGAS stamps on the boss ASC. */
	EGeoDifficulty GetDifficulty() const { return Difficulty; }

	/** Server. Revives every player pawn currently in the world (no-op on a living one, so overlapping calls are free). */
	void RevivePlayers() const;

	/**
	 * Server. A player just went down (death or disconnect). Every death plays out the same way — down for DeathTime,
	 * then back at the checkpoint; the only difference is who waits for whom. Out of a fight the player comes back
	 * alone; during a fight (match in progress) the whole group waits until every player who was alive when the fight
	 * began is down. This is the single death policy — no arena is consulted.
	 */
	void NotifyPlayerDied(APlayableCharacter& Player);

	/** Server. Registers the arena the players count as being in, and so where a death returns them. Written by
	 *  AGeoArenaVolume on entry out of a fight, and by AGeoArena::StartFight, which takes it over for the whole fight
	 *  wherever the players stand. Read only during a respawn, so it never needs to replicate. */
	void SetCurrentArenaTag(FGameplayTag Tag) { CurrentArenaTag = Tag; }
	/** The Arena.* tag the players count as being in. */
	FGameplayTag GetCurrentArenaTag() const { return CurrentArenaTag; }

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

	/** Seconds a downed player stays on the ground before respawning at the checkpoint; also the barrier's opening
	 *  lerp duration on a wipe, so it finishes opening right as the group comes back. */
	UPROPERTY(EditAnywhere, Category = "Fight")
	float DeathTime = 3.f;

	/** Broadcast on every match state transition, on server and clients alike. AGeoArena subscribes to run its fight. */
	FMatchStateChanged OnMatchStateChanged;

	/** Server. Broadcast the moment every fight player is down — DeathTime seconds before the group respawns. */
	FOnWipe OnWipe;

	/** Broadcast on every machine when the difficulty changes. AGeoArena respawns its boss off it, the pads repaint. */
	FOnDifficultyChanged OnDifficultyChanged;

private:
	/**
	 * The one difficulty knob, held here rather than per arena so the pads have a single value to read and no two
	 * encounters can disagree about what the run is set to. Editable as the tuning a session opens on; from there it is
	 * SetDifficulty's alone, which is why it is private.
	 */
	UPROPERTY(EditDefaultsOnly, ReplicatedUsing = OnRep_Difficulty, Category = "Fight")
	EGeoDifficulty Difficulty = EGeoDifficulty::Original;

	UFUNCTION()
	void OnRep_Difficulty();

	/** Arena.* tag a respawn returns to, through the TargetPoint.Entrance points carrying it. Editable as the arena a
	 *  session opens on — the hub, since the players start there before touching any volume; from there it is
	 *  SetCurrentArenaTag's alone, which is why it is private. */
	UPROPERTY(EditDefaultsOnly, Category = "Fight", meta = (Categories = "Arena"))
	FGameplayTag CurrentArenaTag;

	/** Players alive when the current fight began. A wipe is "all of these are down", so late joiners — who are not
	 *  in this snapshot — can neither block a wipe nor trigger one. */
	TArray<TWeakObjectPtr<APlayableCharacter>> FightPlayers;

	FTimerHandle RespawnTimer;

	/** Returns true when no player from the fight-start snapshot is still standing. */
	bool AreFightPlayersDead() const;

	/** Server. Teleports the group to the checkpoint, revives everyone, and stands the match down. Wipe timer callback.
	 */
	void RespawnGroup();

	/** Server. Teleports one player to the checkpoint and revives them. Out-of-fight death timer callback. */
	void RespawnPlayer(APlayableCharacter& Player);
};
