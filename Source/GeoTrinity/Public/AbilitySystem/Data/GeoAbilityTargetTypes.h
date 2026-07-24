// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Abilities/GameplayAbilityTargetTypes.h"

#include "GeoAbilityTargetTypes.generated.h"

/**
 * Whether an interactable query counts a target's own collision radius toward the overlap test.
 * Automatic: decide from the querying source's team — Enemy sources (harmful zones/beams) are
 *   center-only so players can hug the edge; every other source (Player/ally, Neutral, teamless)
 *   includes the radius so beneficial areas reach a target's whole body.
 * IncludeRadius: always add the target's SimpleCollisionRadius.
 * CenterOnly: always test the target's center only.
 */
UENUM(BlueprintType)
enum class ETargetOverlapMode : uint8
{
	Automatic,
	IncludeRadius,
	CenterOnly
};

/**
 * Target data containing client-predicted orientation for projectile abilities.
 * Sent via ServerSetReplicatedTargetData for prediction/rollback support.
 */
USTRUCT(BlueprintType)
struct GEOTRINITY_API FGeoAbilityTargetData : public FGameplayAbilityTargetData
{
	GENERATED_BODY()

	FGeoAbilityTargetData() = default;
	FGeoAbilityTargetData(FVector2D const& InOrigin, float InYaw, float InServerTime, int InSeed) :
		Origin(InOrigin), Yaw(InYaw), ServerSpawnTime(InServerTime), Seed(InSeed)
	{
	}

	UPROPERTY(Transient, BlueprintReadOnly)
	FVector2D Origin{};

	UPROPERTY(Transient, BlueprintReadOnly)
	float Yaw{};

	UPROPERTY(Transient, BlueprintReadOnly)
	float ServerSpawnTime{}; // Server world time in seconds at the moment of the shot

	UPROPERTY(Transient, BlueprintReadOnly)
	int Seed{}; // RNG seed for deterministic variation

	virtual UScriptStruct* GetScriptStruct() const override { return FGeoAbilityTargetData::StaticStruct(); }

	/** Serializes Origin, Yaw, ServerSpawnTime, and Seed for replication. */
	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);

	virtual FString ToString() const override
	{
		return FString::Printf(TEXT("Origin: %s, Yaw: %.2f, ServerSpawnTime : %f, Seed : %i"), *Origin.ToString(), Yaw,
							   ServerSpawnTime, Seed);
	}
};

template <>
struct TStructOpsTypeTraits<FGeoAbilityTargetData> : public TStructOpsTypeTraitsBase2<FGeoAbilityTargetData>
{
	enum
	{
		WithNetSerializer = true
	};
};
