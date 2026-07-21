// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"

#include "GeoArena.generated.h"

class AEnemyCharacter;
class AGeoArenaBarrier;
class AGeoDeployableBase;
class UGeoDeployableManagerComponent;

/**
 * One boss encounter, and it runs itself. It owns its boss (spawn + reset), its barrier, its fight-commit, the boss
 * health bar and the post-victory loot shower. The GameState tells it nothing: on aggro TriggerAggro calls StartFight
 * directly, and it subscribes to AGeoGameState::OnMatchStateChanged to tear the fight down when the match leaves
 * InProgress (a wipe or a victory). Whether this arena's fight is live is the replicated bFighting flag, so clients
 * resolve their own boss bar and the GameState needs no arena pointer at all. What a player's death means stays the
 * GameState's policy; the arena's only part in it is registering its ArenaTag as the respawn CheckpointTag.
 * The fight runs Start (bFighting set, barrier closes, players walk in) -> Commit (players teleported in) -> End;
 * CommitFight and EndFight are virtual so subclasses arm whatever only makes sense once the fight is really live.
 */
UCLASS()
class GEOTRINITY_API AGeoArena : public AActor
{
	GENERATED_BODY()

public:
	AGeoArena();

	/** Registers Boss (clients bind the health bar) and bFighting (clients show/hide it). */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Server. Spawns the boss, or resets the one already standing, for a fresh attempt. */
	void ResetBoss();

	/** Server. Marks this fight live (barrier closes, boss bar shows, checkpoint registered), and starts the commit
	 *  countdown. Called by AGeoEnemyAIController::TriggerAggro the moment this arena's boss is aggroed. Ignored
	 *  while a match is already in progress. */
	virtual void StartFight();
	/** Server. Teleports players to this arena's fight location; the encounter is now fully live. */
	virtual void CommitFight();
	/** Server. Opens the barrier, hides the boss bar, and resets the boss. Runs when the match leaves InProgress. */
	virtual void EndFight();

	/** Returns this arena's boss character; nullptr before BeginPlay spawns it. */
	AEnemyCharacter* GetBoss() const { return Boss; }
	/** Returns true when Enemy is this arena's boss — the enemy whose aggro starts the match. */
	virtual bool IsBoss(AActor const* Enemy) const;

	/** Returns the arena that spawned Boss — every arena spawns its boss with Owner = this. Null when Boss is not one.
	 */
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

protected:
	UFUNCTION()
	void OnRep_Boss();

	/** Server. Spawns this arena's boss and subscribes to the match state so the arena can end its own fight. */
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Arena")
	TSubclassOf<AEnemyCharacter> BossClass;

	/** Barrier sealing this arena while the fight runs. Optional — arenas without one never seal. */
	UPROPERTY(EditAnywhere, Category = "Arena")
	TObjectPtr<AGeoArenaBarrier> Barrier;

	/** Level trigger volume tag; players already inside it keep their position instead of being pulled to the fight
	 *  location on commit. */
	UPROPERTY(EditAnywhere, Category = "Arena")
	FName FightZoneTagName = "FightZone";

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

	/** True while this arena's fight is live. Replicated so every client shows/hides the boss bar off OnRep_bFighting. */
	UPROPERTY(ReplicatedUsing = OnRep_bFighting)
	bool bFighting = false;

	UFUNCTION()
	void OnRep_bFighting();

	/** Shows the boss bar for Boss while bFighting, hides it otherwise. Local HUD only; a no-op on a dedicated server. */
	void ApplyBossBar();

	/** Ends the fight when the match leaves InProgress. Bound to AGeoGameState::OnMatchStateChanged (server only). */
	UFUNCTION()
	void OnMatchStateChanged(FName NewMatchState, FName PreviousMatchState);

	/** The fight is lost but the match still stands: cancels a pending commit and opens the barrier, so it spends the
	 *  DeathTime window opening and finishes right as the group respawns. Bound to AGeoGameState::OnWipe (server only). */
	UFUNCTION()
	void OnWipe(float DeathTime);

	/** Starts the loot shower from the dead boss and stands the match down. Bound to Boss->OnEnemyDefeated. */
	UFUNCTION()
	void OnBossDefeated();

	/** Server. Starts the looping loot shower erupting from LootOrigin. */
	void Loot();
	/** Server. Stops the shower and hands back the pickup slots it borrowed. Runs when any fight starts. */
	void StopLoot();
	/** Spawns one burst of loot pickups from LootOrigin. Timer callback started by Loot(). */
	void SpawnLootBurst();

	FTimerHandle CommitFightTimer;

	FTimerHandle LootTimer;
	FVector LootOrigin = FVector::ZeroVector;

	/** Managers granted an unlimited pickup slot for the loot shower; restored when the next fight starts. */
	TArray<TWeakObjectPtr<UGeoDeployableManagerComponent>> LootBoostedManagers;
	TSubclassOf<AGeoDeployableBase> LootPickupClass;
};
