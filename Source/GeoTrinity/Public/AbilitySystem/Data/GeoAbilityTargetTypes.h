// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Abilities/GameplayAbilityTargetTypes.h"

#include "GeoAbilityTargetTypes.generated.h"

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
	float ServerSpawnTime{}; // server world time (seconds)

	UPROPERTY(Transient, BlueprintReadOnly)
	int Seed{}; // seed pour variations RNG

	virtual UScriptStruct* GetScriptStruct() const override { return FGeoAbilityTargetData::StaticStruct(); }

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
