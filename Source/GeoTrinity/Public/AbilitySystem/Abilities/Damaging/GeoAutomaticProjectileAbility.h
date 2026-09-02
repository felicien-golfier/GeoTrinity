// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Damaging/GeoAutomaticFireAbility.h"
#include "Actor/Projectile/ExternalProjectileParams.h"
#include "CoreMinimal.h"

#include "GeoAutomaticProjectileAbility.generated.h"

enum class EProjectileTarget : uint8;

/**
 * High fire rate ability that spawns projectiles while input is held.
 * Combines the automatic firing loop from UGeoAutomaticFireAbility with projectile spawning.
 */
UCLASS()
class GEOTRINITY_API UGeoAutomaticProjectileAbility : public UGeoAutomaticFireAbility
{
	GENERATED_BODY()

public:
	/** Spawns the cosmetic spread on a client watching an ally fire this ability. */
	virtual void RemoteFireShot(AActor* Avatar, UGeoAbilitySystemComponent* SourceASC) const override;

protected:
	/** Spawns a projectile of ProjectileClass aimed according to the Target mode. Returns true on success. */
	virtual bool ExecuteShot_Implementation() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoAbility")
	FExternalProjectileParams ProjectileParams;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoAbility")
	EProjectileTarget Target;
};
