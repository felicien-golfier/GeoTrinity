// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Abilities/GameplayAbilityTargetTypes.h"

#include "GeoAbilityTargetTypes.generated.h"

/**
 * Target data containing client-predicted orientation for projectile abilities.
 * Sent via ServerSetReplicatedTargetData for prediction/rollback support.
 */
USTRUCT(BlueprintType)
struct GEOTRINITY_API FGeoAbilityTargetData_Orientation : public FGameplayAbilityTargetData
{
	GENERATED_BODY()

	FGeoAbilityTargetData_Orientation() = default;
	FGeoAbilityTargetData_Orientation(const FVector2D& InOrigin, float InYaw) : Origin(InOrigin), Yaw(InYaw) {}

	UPROPERTY(BlueprintReadWrite)
	FVector2D Origin{};

	UPROPERTY(BlueprintReadWrite)
	float Yaw{0.f};

	virtual UScriptStruct* GetScriptStruct() const override
	{
		return FGeoAbilityTargetData_Orientation::StaticStruct();
	}

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);

	virtual FString ToString() const override
	{
		return FString::Printf(TEXT("Origin: %s, Yaw: %.2f"), *Origin.ToString(), Yaw);
	}
};

template<>
struct TStructOpsTypeTraits<FGeoAbilityTargetData_Orientation>
	: public TStructOpsTypeTraitsBase2<FGeoAbilityTargetData_Orientation>
{
	enum
	{
		WithNetSerializer = true
	};
};
