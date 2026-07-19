// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Actor/Projectile/GeoProjectile.h"
#include "CoreMinimal.h"

#include "GeoShieldBurstProjectile.generated.h"

/** Replication bundle that captures the full post-bounce state (location, velocity, sphere radius) for simulated
 * clients. */
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
	/** Sets default bounce parameters and replication policy for BounceSnapshot. */
	AGeoShieldBurstProjectile();
	/** Registers BounceSnapshot for replication with OnRep_BounceSnapshot. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Shield magnitude applied to allies on contact. Scales up with each enemy bounce. */
	FScalableFloat ShieldAmount = 0.f;
	/** Factor applied to both ShieldAmount and sphere radius each time the projectile bounces off an enemy. */
	float EnemyBounceMultiplier;
	float SphereRadiusToAdd;
	float ShieldAmounToAdd;

protected:
	/** Sound played each time the projectile bounces, off a wall or an enemy. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoProjectile|Audio")
	FGeoSoundEntry BounceSound;

	/** Extra pitch multiplier for BounceSound, evaluated against the sphere's current scaled radius. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoProjectile|Audio")
	TObjectPtr<UCurveFloat> BounceSoundSizePitchCurve;

	/** Extends base setup to bind the wall-bounce delegate. */
	virtual void InitProjectileLife() override;
	/**
	 * On enemy overlap: plays BounceSound on every machine; on the server also reflects the projectile and scales
	 * ShieldAmount and sphere radius by EnemyBounceMultiplier.
	 * On ally overlap (server): applies ShieldAmount as a shield effect and ends the projectile life.
	 */
	virtual void HandleValidOverlap(AActor* OtherActor) override;

	/** Returns false for AGeoWall (passes through the Square's own deployable walls) and for the same hostile actor
	 * within 0.5 s (prevents double-hit on glancing overlaps). */
	virtual bool IsValidOverlap(AActor* OtherActor) override;

	/** Extends base pitch with an extra factor from BounceSoundSizePitchCurve, evaluated at the sphere's current
	 * scaled radius, so bigger bursts pitch differently. */
	virtual float GetPitch(FGeoSoundEntry const& Entry) const override;

	/** Teleports the projectile to the post-bounce state and updates the Niagara radius parameter on simulated clients.
	 */
	UFUNCTION()
	void OnRep_BounceSnapshot();

	/** Sets the Niagara "User.Bullet_Radius" parameter. Called on every machine — the host updates it directly after a
	 * bounce (where OnRep never runs), simulated clients via OnRep_BounceSnapshot. */
	void UpdateVisualRadius(float Radius) const;

private:
	/** Plays BounceSound, then on the server records the post-bounce location, velocity, and radius in
	 * BounceSnapshot for replication to simulated clients. */
	UFUNCTION()
	void OnWallBounce(FHitResult const& ImpactResult, FVector const& ImpactVelocity);

	UPROPERTY(ReplicatedUsing = OnRep_BounceSnapshot)
	FShieldBounceSnapshot BounceSnapshot;

	TWeakObjectPtr<AActor> LastOverlapHostileActor;
	float LastOverlapTime = 0.f;
};
