// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once
#include "GameplayPrediction.h"
#include "GameplayTagContainer.h"

#include "AbilityPayload.generated.h"

/**
 * Snapshot of all data needed to replicate an ability shot or pattern spawn across machines.
 * Stored as StoredPayload on ability instances. Also carried by UPattern subclasses.
 * Always use the fields here instead of calling ability helper functions (GetAvatarActor, etc.),
 * as the payload may intentionally differ from what ActorInfo reports on the server.
 */
USTRUCT(BlueprintType)
struct GEOTRINITY_API FAbilityPayload
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly)
	FVector2D Origin{}; // World-space XY position at spawn time
	UPROPERTY(Transient, BlueprintReadOnly)
	float Yaw{}; // Character facing yaw in degrees at spawn time

	UPROPERTY(Transient, BlueprintReadOnly)
	float ServerSpawnTime{}; // Server world time in seconds at spawn time

	UPROPERTY(Transient, BlueprintReadOnly)
	int Seed{}; // RNG seed for deterministic variation

	UPROPERTY(Transient, BlueprintReadOnly)
	int AbilityLevel{};

	// TODO: optimise AbilityTag : remove from payload and set only once on Pattern Creation.
	UPROPERTY(Transient, BlueprintReadOnly)
	FGameplayTag AbilityTag{};

	UPROPERTY(Transient, BlueprintReadOnly)
	AActor* Owner{nullptr};

	UPROPERTY(Transient, BlueprintReadOnly)
	AActor* Instigator{nullptr};

	/**
	 * The single hit notification this shot is allowed, consumed by the first target it connects with
	 * (GeoASLib::NotifyAbilityHit). Every carrier one shot spawns shares it, so the five projectiles of a spread report
	 * one hit between them, and a beam ticking over four targets reports one.
	 *
	 * Deliberately outside reflection: it must not cross the wire with the payload (PatternStartMulticast), and a
	 * machine that only received a copy is not the one that fired. A payload without it — a cosmetic remote-fire one, a
	 * replicated copy, a default-constructed one — reports nothing.
	 */
	TSharedPtr<bool> HitNotified;

	/**
	 * Opens a fresh, unspent hit notification for a new shot. Called once per shot — at payload creation, and again
	 * every time the payload is re-stamped for the next shot of a held trigger — so an auto-fire burst reports one hit
	 * per shot rather than one for the whole hold.
	 */
	void OpenHitNotification() { HitNotified = MakeShared<bool>(false); }
};

/**
 * Polymorphic base for pattern-specific data carried through PatternStartMulticast alongside FAbilityPayload.
 * A pattern that needs extra replicated data defines its own FPatternData subclass with UPROPERTY fields, fills it on
 * the server in UPatternAbility::CreatePatternData(), and reads it back in InitPattern via PatternData.GetPtr<T>().
 * Empty by default — most patterns need no extra data and pass an unset TInstancedStruct.
 */
USTRUCT(BlueprintType)
struct GEOTRINITY_API FPatternData
{
	GENERATED_BODY()
};
