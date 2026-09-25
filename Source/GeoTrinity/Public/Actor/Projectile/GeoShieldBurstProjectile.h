// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Data/GeoSoundRow.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "CoreMinimal.h"
#include "ScalableFloat.h"

#include "GeoShieldBurstProjectile.generated.h"

/** Replication bundle that captures the full state (location, velocity, sphere radius) for simulated clients: seeded at
 * spawn, refreshed on every bounce and every ResyncInterval. */
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

	/** Enemy bounces so far. A client plays BounceSound when it grows past the bounces it already predicted itself. */
	UPROPERTY()
	int32 EnemyBounceCount = 0;

	/** Server time the state above is true at, so a client can fast-forward it by the time it spent in transit. */
	UPROPERTY()
	float ServerTime = 0.f;
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
	/** On the server, refreshes BounceSnapshot every ResyncInterval, so a client copy that mispredicted an enemy bounce
	 * is pulled back even when no real bounce follows. */
	virtual void Tick(float DeltaSeconds) override;

	/** Shield magnitude applied to allies on contact. Scales up with each enemy bounce. */
	float ShieldAmount = 0.f;
	/** Additive growth per enemy bounce: each bounce adds this fraction of the base ShieldAmount and sphere radius
	 * (fixed increment snapshotted at spawn), so growth is linear rather than compounding. */
	float EnemyBounceAdditiveMultiplier = 0.f;
	float SphereRadiusToAdd = 0.f;
	float ShieldAmountToAdd = 0.f;

protected:
	/** Sound played each time the projectile bounces off an enemy. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoProjectile|Audio")
	FGeoSoundEntry BounceSound;

	/** Extra pitch multiplier for every sound this burst plays, evaluated against its current scaled radius. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoProjectile|Audio")
	TObjectPtr<UCurveFloat> BounceSoundSizePitchCurve;

	/** How long the burst blinks before its lifespan runs out, like a deployable about to expire. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoProjectile|GameFeel")
	float BlinkDuration = 2.f;

	/** Extends base setup to bind the wall-bounce delegate and schedule the blink over the last BlinkDuration of the
	 * lifespan. On the server seeds BounceSnapshot with the spawn state; on clients applies the snapshot the server
	 * already sent, which the base BeginPlay (DefaultParams radius, velocity reset to the spawn direction) has just
	 * overwritten. */
	virtual void InitProjectileLife() override;
	/**
	 * On enemy overlap (every machine): plays BounceSound and reflects the projectile, so a client bounces on its own
	 * instead of flying through the enemy until the snapshot arrives. The server alone grows ShieldAmount and the sphere
	 * radius by EnemyBounceAdditiveMultiplier (the growth reaches clients through the snapshot).
	 * On ally overlap: flashes the ally on every machine; the server applies ShieldAmount as a shield effect and ends
	 * the projectile life, which reaches clients through bEndedOnServer — a client's copy never ends itself on an ally
	 * its lagging view merely shows in the way.
	 */
	virtual void HandleValidOverlap(AActor* OtherActor, UGeoAbilitySystemComponent* OwnerASC,
									UGeoAbilitySystemComponent* TargetASC) override;

	/** Returns false for friendly deployables (the burst flies through the team's walls, turrets and zones instead
	 * of spending its shield on them), for the same hostile actor within 0.5 s (prevents double-hit on glancing
	 * overlaps), and for a hostile the burst is moving away from — only what it flies into bounces it, which also keeps
	 * a client's snapshot teleport into an enemy from reflecting an already reflected velocity. Hostile and neutral
	 * deployables stay valid, so the burst still bounces off them. */
	virtual bool IsValidOverlap(AActor* OtherActor, UGeoAbilitySystemComponent*& OutOwnerASC,
								UGeoAbilitySystemComponent*& OutTargetASC) override;

	/** Applies the new snapshot on simulated clients, playing BounceSound first when it carries an enemy bounce this
	 * client did not predict. Drops a snapshot taken before an enemy bounce this client already predicted. */
	UFUNCTION()
	void OnRep_BounceSnapshot();

	/** Matches everything that follows the burst's size to RadiusOverride: the bullet visual, and the pitch of its
	 * sounds through BounceSoundSizePitchCurve, so bigger bursts sound different. Called on every machine — the host
	 * updates it directly after a bounce (where OnRep never runs), simulated clients via OnRep_BounceSnapshot. */
	void UpdateSizeFX(float Radius) const;

	/** How often the server refreshes BounceSnapshot between bounces. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoProjectile|Network")
	float ResyncInterval = 0.25f;

private:
	/** Sends the projectile straight out along the wall normal, on every machine so clients bounce exactly like the
	 * server; the server also records the result in BounceSnapshot. */
	UFUNCTION()
	void OnWallBounce(FHitResult const& ImpactResult, FVector const& ImpactVelocity);

	/** This machine's current flight state, stamped with the server time it is true at. */
	FShieldBounceSnapshot CaptureSnapshot(int32 EnemyBounceCount) const;

	/** Client: moves the projectile to BounceSnapshot, then fast-forwards it through the snapshot's transit time
	 * (capped at MaxLatencyCompensation) with the movement component's own simulation, walls and enemy bounces
	 * included, so it lands where the server's burst is now rather than where it was. The bullet visual slides over
	 * from where it was drawn instead of popping. */
	void ApplyBounceSnapshot();

	UPROPERTY(ReplicatedUsing = OnRep_BounceSnapshot)
	FShieldBounceSnapshot BounceSnapshot;

	/** Client only: the newest flight state this client trusts — the last snapshot it accepted, or an enemy bounce it
	 * predicted since. */
	FShieldBounceSnapshot ClientSnapshot;

	TWeakObjectPtr<AActor> LastOverlapHostileActor;
	float LastOverlapTime = 0.f;
};
