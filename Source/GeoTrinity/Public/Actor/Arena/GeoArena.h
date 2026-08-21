// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "World/GeoBackgroundPulseComponent.h"

#include "GeoArena.generated.h"

class AEnemyCharacter;
class AGeoArenaBarrier;
class AGeoDeployableBase;
class UGeoDeployableManagerComponent;
class UGeoReloadAbility;

/**
 * One boss encounter, and it runs itself. It owns its boss and its adds (spawn + reset), its barrier, its fight-commit,
 * the boss health bar and the post-victory loot shower. The GameState tells it nothing: on aggro TriggerAggro calls
 * StartFight directly, and it subscribes to AGeoGameState::OnMatchStateChanged to tear the fight down when the match
 * leaves InProgress (a wipe or a victory). Whether this arena's fight is live is the replicated bFighting flag, so
 * clients resolve their own boss bar and the GameState needs no arena pointer at all. What a player's death means
 * stays the GameState's policy; the arena's only part in it is taking over the GameState's CurrentArenaTag for as
 * long as its fight runs, which is what makes a wipe come back here rather than wherever the players last walked.
 * The fight runs Start (bFighting set, barrier closes, players walk in) -> Commit (players teleported in) -> End;
 * CommitFight and EndFight are virtual so subclasses arm whatever only makes sense once the fight is really live.
 */
UCLASS()
class GEOTRINITY_API AGeoArena : public AActor
{
	GENERATED_BODY()

public:
	/** Enables actor replication. */
	AGeoArena();

	/** Registers Boss (clients bind the health bar) and bFighting (clients show/hide it). */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Server. Spawns the boss, or resets the one already standing, for a fresh attempt. */
	void ResetBoss();

	/** Opens or seals the barrier, if this arena has one. Barrier state is owned by the fight-lifecycle functions. */
	void SetBarrierClosed(bool bClosed) const;

	/** Server. Marks this fight live (barrier closes, boss bar shows, current arena taken over), and starts the commit
	 *  countdown. Called by AGeoEnemyAIController::TriggerAggro the moment this arena's boss is aggroed. Ignored
	 *  while a match is already in progress. */
	virtual void StartFight();
	/** Server. Teleports players to this arena's fight location — bar those already standing in one of its
	 *  AGeoArenaVolumes — and the encounter is now fully live. */
	virtual void CommitFight();
	/** Server. Opens the barrier, hides the boss bar, and resets the boss — unless the boss was defeated, in which case
	 *  it is left dead until RespawnBoss. Runs on *every* arena when the match leaves InProgress, not just the one that
	 *  was fighting: aggro is not exclusive (a hit aggroes a boss whatever the match state), so a boss pulled while
	 *  another arena's fight was live has to be put back too. */
	virtual void EndFight();

	/** Server. Clears the defeated state and spawns the boss again — the only way back after a victory, and how this
	 *  arena picks up a difficulty change: bound to AGeoGameState::OnDifficultyChanged, since the level a boss runs at
	 *  is stamped on its ASC when it spawns and nothing re-levels a boss already standing. */
	UFUNCTION()
	void RespawnBoss();
	/** Server. RespawnBoss on every arena whose boss was defeated. What a respawn button drives. */
	static void RespawnAllBosses(UObject const* WorldContextObject);

	/** Returns this arena's boss character; nullptr before BeginPlay spawns it. */
	AEnemyCharacter* GetBoss() const { return Boss; }
	/** Returns true when Enemy is this arena's boss — the enemy whose aggro starts the match. */
	virtual bool IsBoss(AActor const* Enemy) const;

	/** Returns this boss's owning arena via AEnemyCharacter::Arena. Null if Boss is null or not an AEnemyCharacter. */
	static AGeoArena* GetArenaOfBoss(AActor const* Boss);
	/** Returns the arena whose fight is currently live, or null when no fight runs. At most one arena is ever fighting. */
	static AGeoArena* GetFightingArena(UObject const* WorldContextObject);

	/**
	 * Names this encounter. Every AGeoTargetPoint it uses carries this tag alongside its TargetPoint.* purpose, so a
	 * new arena only needs its own Arena.* tag — the purposes are shared by every arena and live in code.
	 * Editor-authored: add the tag in the project settings, no native constant needed.
	 */
	UPROPERTY(EditAnywhere, Category = "Arena")
	FGameplayTag ArenaTag;

	/** Background lattice behaviour while this arena's fight is live; the pulse driver's own authored mode returns
	 * when it ends. Cosmetic only — it is applied off bFighting, so it lands on every machine and replicates nothing
	 * of its own. */
	UPROPERTY(EditAnywhere, Category = "Arena")
	EGeoPulseMode PulseMode = EGeoPulseMode::Actors;

protected:
	UFUNCTION()
	void OnRep_Boss();

	/** Server. Spawns this arena's boss and subscribes to the match state so the arena can end its own fight. */
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Arena")
	TSubclassOf<AEnemyCharacter> BossClass;

	/** Extra enemies spawned with the boss, one per entry, at the arena's TargetPoint.AddSpawn points. They never gate
	 *  the fight: IsBoss ignores them, they get no health bar, and they are wiped the moment the boss dies. */
	UPROPERTY(EditAnywhere, Category = "Arena")
	TArray<TSubclassOf<AEnemyCharacter>> AddClasses;

	/** Barrier sealing this arena while the fight runs. Optional — arenas without one never seal. */
	UPROPERTY(EditAnywhere, Category = "Arena")
	TObjectPtr<AGeoArenaBarrier> Barrier;

	/** Seconds between loot pickup bursts after the boss dies. */
	UPROPERTY(EditAnywhere, Category = "Loot")
	float LootSpawnInterval = 0.1f;
	/** Pickups spawned per burst. */
	UPROPERTY(EditAnywhere, Category = "Loot")
	int32 LootPickupsPerBurst = 1;
	/** Scatter radius around the dead boss the pickups are launched to. */
	UPROPERTY(EditAnywhere, Category = "Loot")
	float LootMaxRadius = 1500.f;

private:
	UPROPERTY(ReplicatedUsing = OnRep_Boss)
	TObjectPtr<AEnemyCharacter> Boss;

	/** The live adds. Server-only bookkeeping — each add replicates itself as a normal actor, so clients never need the
	 *  list. */
	UPROPERTY()
	TArray<TObjectPtr<AEnemyCharacter>> Adds;

	/** Server. Clears the standing adds and spawns a fresh set from AddClasses. Called by ResetBoss. */
	void ResetAdds();
	/** Server. Destroys every standing add and empties the list. */
	void DestroyAdds();

	/** True while this arena's fight is live. Replicated so every client shows/hides the boss bar off OnRep_bFighting. */
	UPROPERTY(ReplicatedUsing = OnRep_bFighting)
	bool bFighting = false;

	/** True once this arena's boss has been beaten. Blocks the EndFight respawn until RespawnBoss clears it, so a
	 *  victory doesn't drop a fresh boss on the players looting the corpse. Server-only state. */
	bool bBossDefeated = false;

	UFUNCTION()
	void OnRep_bFighting();

	/** Every local cosmetic that keys off this arena's fight state. Called from the three places bFighting changes —
	 * StartFight and EndFight on the server, OnRep_bFighting on the clients — so both halves land on every machine. */
	void ApplyFightVisuals();

	/** Shows the boss bar for Boss while bFighting, hides it otherwise. Local HUD only; a no-op on a dedicated server. */
	void ApplyBossBar();

	/** Hands PulseMode to the background lattice while this fight runs, and gives the driver its own mode back when it
	 * ends. Silently does nothing where no driver exists, which is every dedicated server. */
	void ApplyBackgroundPulse() const;

	/** Ends this arena's fight — fighting or not — when the match leaves InProgress. Bound to
	 *  AGeoGameState::OnMatchStateChanged (server only). */
	UFUNCTION()
	void OnMatchStateChanged(FName NewMatchState, FName PreviousMatchState);

	/** The fight is lost but the match still stands: cancels a pending commit and opens the barrier, so it spends the
	 *  DeathTime window opening and finishes right as the group respawns. Bound to AGeoGameState::OnWipe (server only). */
	UFUNCTION()
	void OnWipe(float DeathTime);

	/** Starts the loot shower from the dead boss and stands the match down. Bound to Boss->OnEnemyDefeated. */
	UFUNCTION()
	void OnBossDefeated();

	/** Server. Resolves the reload ability that configures the pickups, then starts the looping loot shower erupting
	 *  from LootOrigin. Declines to start the timer when no such ability is registered. */
	void Loot();
	/** Server. Stops the shower and hands back the pickup slots it borrowed. Runs when any fight starts. */
	void StopLoot();
	/** Spawns one burst of loot pickups from LootOrigin. Timer callback started by Loot(). */
	void SpawnLootBurst();

	FTimerHandle CommitFightTimer;

	FTimerHandle LootTimer;
	FVector LootOrigin = FVector::ZeroVector;

	/** Pickup configuration for the running shower, resolved once in Loot() rather than re-scanned on every burst. */
	UGeoReloadAbility const* LootReloadCDO = nullptr;
	FGameplayTag LootReloadTag;

	/** Managers granted an unlimited pickup slot for the loot shower; restored when the next fight starts. */
	TArray<TWeakObjectPtr<UGeoDeployableManagerComponent>> LootBoostedManagers;
	TSubclassOf<AGeoDeployableBase> LootPickupClass;
};
