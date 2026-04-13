// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Actor/Projectile/GeoProjectile.h"
#include "CoreMinimal.h"

#include "GeoShieldBurstProjectile.generated.h"

/**
 * Projectile launched by the Square's passive ability when its gauge fills.
 * Bounces off enemies (multiplying ShieldAmount) and gives shield on ally contact.
 */
UCLASS()
class GEOTRINITY_API AGeoShieldBurstProjectile : public AGeoProjectile
{
	GENERATED_BODY()

public:
	AGeoShieldBurstProjectile();

	float ShieldAmount = 0.f;

protected:
	virtual void HandleValidOverlap(AActor* OtherActor) override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "ShieldBurst", meta = (AllowPrivateAccess = true))
	float EnemyBounceMultiplier = 1.5f;
};
