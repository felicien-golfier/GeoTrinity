// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"

#include "GeoArenaVolume.generated.h"

class APawn;
class UBoxComponent;

/**
 * Box naming which arena the players standing in it belong to. Walking into one registers its ArenaTag as the
 * GameState's current arena — the tag a respawn returns to — but only out of a fight: a live fight owns the current
 * arena outright (AGeoArena::StartFight writes it), wherever the players happen to be standing, so the volume
 * refuses to write while a match is in progress. The same volume answers the fight-commit question through
 * IsPawnInside: a player already inside one keeps their position instead of being pulled to
 * TargetPoint.FightLocation.
 *
 * Several volumes may carry the same ArenaTag — an arena spread over two rooms, or an approach worth checkpointing
 * ahead of the fight floor. Nothing here tracks *which* volume a player is in, only which arena was entered last, so
 * extra volumes need no reconciling: begin overlap only, no end overlap, no per-player state. Leaving every volume
 * therefore leaves the tag where it was, which is what a respawn point wants.
 */
UCLASS()
class GEOTRINITY_API AGeoArenaVolume : public AActor
{
	GENERATED_BODY()

public:
	/** Creates the TriggerBox as the root component. The overlap callback is bound in BeginPlay. */
	AGeoArenaVolume();

	/** True when Pawn stands inside any volume carrying ArenaTag. */
	static bool IsPawnInside(UObject const* WorldContextObject, APawn const& Pawn, FGameplayTag ArenaTag);

protected:
	/** Binds the overlap registering the current arena, and claims it for anyone already standing inside. Server only —
	 *  nothing reads the tag on a client. */
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GeoArena")
	TObjectPtr<UBoxComponent> TriggerBox;

	/** Names the encounter this volume belongs to — the same Arena.* tag its AGeoArena and target points carry. */
	UPROPERTY(EditAnywhere, Category = "GeoArena", meta = (Categories = "Arena"))
	FGameplayTag ArenaTag;

private:
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
						int32 OtherBodyIndex, bool bFromSweep, FHitResult const& SweepResult);

	/** Server. Makes this volume's arena the current one, unless a fight is running — that one owns it outright. */
	void ClaimCurrentArena() const;
};
