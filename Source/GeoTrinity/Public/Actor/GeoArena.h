// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"

#include "GeoArena.generated.h"

class AEnemyCharacter;
class AGeoArenaBarrier;

/**
 * One boss encounter: the boss it owns, the target-point tags its players are moved between, and its barrier.
 * AGeoGameState runs a single match at a time and reads the encounter off whichever arena is active, so a level can
 * hold several arenas without the match machinery knowing any of them by class.
 * The fight runs Start (barrier closes, players walk in) -> Commit (players teleported in) -> End; subclasses hook
 * those to arm whatever only makes sense once the fight is really live.
 */
UCLASS()
class GEOTRINITY_API AGeoArena : public AActor
{
	GENERATED_BODY()

public:
	AGeoArena();

	/** Registers the boss pointer, which clients need to bind the boss health bar. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Server. Spawns the boss, or resets the one already standing, for a fresh attempt. */
	void ResetBoss();

	/** Server. Closes the barrier. Players are still walking in — the fight is not committed yet. */
	virtual void StartFight();
	/** Server. Players have been teleported to this arena's fight location; the encounter is now fully live. */
	virtual void CommitFight();
	/** Server. Opens the barrier and stands the encounter down. */
	virtual void EndFight();

	AEnemyCharacter* GetBoss() const { return Boss; }
	/** Returns true when Enemy is this arena's boss — the enemy whose aggro starts the match. */
	virtual bool IsBoss(AActor const* Enemy) const;

	/** Returns the arena that spawned Boss — every arena spawns its boss with Owner = this. Null when Boss is not one.
	 */
	static AGeoArena* GetArenaOfBoss(AActor const* Boss);

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

	/** Server. Spawns this arena's boss so it exists to be aggroed. */
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Arena")
	TSubclassOf<AEnemyCharacter> BossClass;

	/** Barrier sealing this arena while the fight runs. Optional — arenas without one never seal. */
	UPROPERTY(EditAnywhere, Category = "Arena")
	TObjectPtr<AGeoArenaBarrier> Barrier;

private:
	UPROPERTY(ReplicatedUsing = OnRep_Boss)
	TObjectPtr<AEnemyCharacter> Boss;
};
