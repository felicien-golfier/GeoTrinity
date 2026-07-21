// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"

#include "GeoArena.generated.h"

class AEnemyCharacter;
class AGeoArenaBarrier;
class APlayableCharacter;

/**
 * One boss encounter: the boss it owns, the target points its players are moved between, its barrier, and what
 * happens when a player dies in it. AGeoGameState only tracks which arena is active and whether a match is running,
 * so a level can hold several arenas without the match machinery knowing any of them by class.
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

	/** Server. Closes the barrier and starts the commit countdown. Players are still walking in. */
	virtual void StartFight();
	/** Server. Teleports players to this arena's fight location; the encounter is now fully live. */
	virtual void CommitFight();
	/** Server. Opens the barrier and stands the encounter down. */
	virtual void EndFight();

	/**
	 * Server. A player just went down in this arena, by death or by disconnecting. Base: leaves them down until
	 * nobody is left standing, then puts the whole group back at the entrance after DeathTime.
	 */
	virtual void RespawnPlayer(APlayableCharacter& Player);

	/** Returns this arena's boss character; nullptr before BeginPlay spawns it. */
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

	/** Seconds the group stays down after a wipe before this arena puts it back on its feet. */
	UPROPERTY(EditAnywhere, Category = "Arena")
	float DeathTime = 3.f;

	/** Level trigger volumes; players already inside are left where they are instead of being teleported. */
	UPROPERTY(EditAnywhere, Category = "Arena")
	FName FightZoneTagName = "FightZone";
	UPROPERTY(EditAnywhere, Category = "Arena")
	FName EntranceZoneTagName = "EntranceZone";

private:
	/** Server. Moves players to this arena's points carrying PurposeTag, skipping anyone inside ExemptZoneName. */
	void TeleportPlayersTo(FGameplayTag PurposeTag, FName ExemptZoneName) const;

	/** Returns true when no player is left standing. */
	bool AreAllPlayersDead() const;

	/** Server. Puts the downed group back on its feet at this arena's entrance and stands the match down. */
	void RespawnGroup();

	UPROPERTY(ReplicatedUsing = OnRep_Boss)
	TObjectPtr<AEnemyCharacter> Boss;

	FTimerHandle CommitFightTimer;
	FTimerHandle RespawnTimer;
};
