// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once
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
};
