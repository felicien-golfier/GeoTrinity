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
	float Radius = 0.f;
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

	/** Shield magnitude applied to allies on contact. Scales up with each enemy bounce. */
	FScalableFloat ShieldAmount = 0.f;
	/** Factor applied to both ShieldAmount and sphere radius each time the projectile bounces off an enemy. */
	float EnemyBounceMultiplier;

protected:
	/** Extends base setup to bind the wall-bounce delegate on the server. */
	virtual void InitProjectileLife() override;
	/**
	 * On enemy overlap: reflects the projectile and scales ShieldAmount and sphere radius by EnemyBounceMultiplier.
	 * On ally overlap: applies ShieldAmount as a shield effect and ends the projectile life.
	 */
	virtual void HandleValidOverlap(AActor* OtherActor) override;

	virtual bool IsValidOverlap(AActor* OtherActor) override;

	/** Teleports the projectile to the post-bounce state and updates the Niagara radius parameter on simulated clients.
	 */
	UFUNCTION()
	void OnRep_BounceSnapshot();

private:
	/** Records the post-bounce location, velocity, and radius in BounceSnapshot for replication to simulated clients.
	 */
	UFUNCTION()
	void OnWallBounce(FHitResult const& ImpactResult, FVector const& ImpactVelocity);

	UPROPERTY(ReplicatedUsing = OnRep_BounceSnapshot)
	FShieldBounceSnapshot BounceSnapshot;

	TWeakObjectPtr<AActor> LastOverlapHostileActor;
	float LastOverlapTime = 0.f;
};
