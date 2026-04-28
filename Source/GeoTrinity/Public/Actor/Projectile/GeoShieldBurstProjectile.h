// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Actor/Projectile/GeoProjectile.h"
#include "CoreMinimal.h"

#include "GeoShieldBurstProjectile.generated.h"

USTRUCT()
struct FShieldBounceSnapshot
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	UPROPERTY()
	FVector Velocity = FVector::ZeroVector;

	UPROPERTY()
	float Radius;
};

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
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	float ShieldAmount = 0.f;
	float EnemyBounceMultiplier;

protected:
	virtual void InitProjectileLife() override;
	virtual void HandleValidOverlap(AActor* OtherActor) override;

	UFUNCTION()
	void OnRep_BounceSnapshot();

private:
	UFUNCTION()
	void OnWallBounce(FHitResult const& ImpactResult, FVector const& ImpactVelocity);

	UPROPERTY(ReplicatedUsing = OnRep_BounceSnapshot)
	FShieldBounceSnapshot BounceSnapshot;
};
