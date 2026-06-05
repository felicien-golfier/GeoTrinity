// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Pattern/Pattern.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StructUtils/InstancedStruct.h"
#include "Tool/Team.h"

#include "GeoProjectile.generated.h"

class UGeoAbilitySystemComponent;
class UProjectileMovementComponent;
class UNiagaraSystem;
class USphereComponent;
class USceneComponent;
class USoundBase;
class UAudioComponent;
class UPrimitiveComponent;
struct FHitResult;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectileEndLife, AGeoProjectile*, Projectile);
UCLASS()
class GEOTRINITY_API AGeoProjectile : public AActor
{
	GENERATED_BODY()
public:
	AGeoProjectile();
	/** Registers PredictionKeyId for replication. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	/**
	 * Server-spawned projectiles are hidden from their owning client so the client's locally predicted
	 * copy remains authoritative for that player's view. All other clients receive normal replication.
	 */
	virtual bool IsNetRelevantFor(AActor const* RealViewer, AActor const* ViewTarget,
								  FVector const& SrcLocation) const override;
	/**
	 * For non-pooled projectiles: re-applies movement on the server (Blueprint construction resets velocity),
	 * and calls InitProjectileLife on clients. Also destroys the matching predicted projectile on the owning client
	 * when CVarReplaceLocalProjectiles is enabled.
	 */
	virtual void BeginPlay() override;
	/** Guards against double-ending by checking bIsEnding before calling EndProjectileLife. */
	virtual void LifeSpanExpired() override;
	/** Checks cumulative travel distance each tick and calls EndProjectileLife when DistanceSpanSqr is exceeded. */
	virtual void Tick(float DeltaSeconds) override;

	/**
	 * Binds hit/overlap delegates, starts the lifespan timer, records the initial position, and applies movement.
	 * Called by pooled variants via Init(); called in BeginPlay for non-pooled replicated projectiles on clients.
	 */
	UFUNCTION()
	virtual void InitProjectileLife();

	/**
	 * Fast-forwards the projectile's position by TimeDelta seconds of movement.
	 * Used on the server to align a newly spawned authoritative projectile with a client-predicted one
	 * that has already been flying for the duration of the owning client's ping.
	 *
	 * @param TimeDelta  Elapsed time in seconds to advance (typically half round-trip ping).
	 */
	void AdvanceProjectile(float TimeDelta);

	/**
	 * Overrides the maximum travel distance for this projectile instance.
	 *
	 * @param Distance  Maximum travel distance in cm before the projectile ends its life.
	 */
	UFUNCTION(BlueprintCallable)
	void OverrideDistanceSpan(float Distance);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(BlueprintReadOnly)
	TArray<TInstancedStruct<struct FEffectData>> EffectDataArray;

	UPROPERTY()
	TObjectPtr<USceneComponent> HomingTargetSceneComponent;

	UPROPERTY(BlueprintReadOnly)
	FAbilityPayload Payload;

	UPROPERTY(Replicated)
	int16 PredictionKeyId = 0;

	FOnProjectileEndLife OnProjectileEndLifeDelegate;

protected:
	/**
	 * Returns true when OtherActor is a valid hit target for this projectile.
	 * Default implementation checks team attitude bitmask. Override to restrict targeting (e.g. ground only).
	 */
	virtual bool IsValidOverlap(AActor* OtherActor);

	/**
	 * Called from OnSphereOverlap after IsValidOverlap passes. Override to customise hit behaviour.
	 * Default: applies EffectDataArray to target, calls OnProjectileHit, ends projectile life.
	 */
	virtual void HandleValidOverlap(AActor* OtherActor);

	/** Dispatches overlap events to IsValidOverlap then HandleValidOverlap. */
	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
						 UPrimitiveComponent* OtherOverlappedComponent, int32 OtherBodyIndex, bool bFromSweep,
						 FHitResult const& SweepResult);
	/** Called on a physics blocking hit (wall or environment). Default is a no-op; override to implement bounce
	 * behaviour. */
	UFUNCTION()
	virtual void OnSphereHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
							 FVector NormalImpulse, FHitResult const& Hit);

	/** Plays ImpactSound and spawns ImpactEffect (Niagara) at the actor's current location. */
	UFUNCTION(BlueprintCallable)
	virtual void PlayImpactFx() const;

	/**
	 * Called on every machine that has this projectile when it hits a valid actor.
	 * Override in Blueprint to apply a hit flash or other cosmetic reaction on the HitActor's mesh.
	 *
	 * @param HitActor  The actor that was struck. Cast to GeoCharacter to access its mesh.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Projectile|GameFeel")
	void OnProjectileHit(AActor* HitActor);

	/** Called when the projectile's life ends (distance exceeded, lifespan expired, or valid hit). Destroys the actor
	 * by default. */
	virtual void EndProjectileLife();
	/** Configures the UProjectileMovementComponent from the projectile's UPROPERTY settings. */
	void InitProjectileMovementComponent();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USphereComponent> Sphere;

	UPROPERTY()
	TObjectPtr<UAudioComponent> LoopingSoundComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoProjectile",
			  meta = (Bitmask, BitmaskEnum = "/Script/GeoTrinity.ETeamAttitudeBitflag", AllowPrivateAccess = true))
	int32 OverlapAttitude = TeamAttitudeMask::HostileOrNeutral;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoProjectile",
			  meta = (Tooltip = "Safe guard in case distance check fails", AllowPrivateAccess = true))
	float LifeSpanInSec = 30.f;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoProjectile", meta = (AllowPrivateAccess = true))
	bool bUseGeneralSpellDistance = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoProjectile",
			  meta = (ClampMin = "0", AllowPrivateAccess = true, EditCondition = "!bUseGeneralSpellDistance",
					  EditConditionHides = "true", UIMin = "0"))
	float DistanceSpan = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoProjectile", meta = (AllowPrivateAccess = true))
	bool bCanOverlapInstigator = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoProjectile",
			  meta = (EditConditionHides, EditCondition = bCanOverlapInstigator, AllowPrivateAccess = true))
	float LifeTimeThresholdBeforeOverlapSelf = 0.2f;

	bool bIsEnding{false};

	FVector InitialPosition;
	float DistanceSpanSqr;

	/** Cosmetic (let the juice flow) **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoProjectile", meta = (AllowPrivateAccess = true))
	TObjectPtr<UNiagaraSystem> ImpactEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoProjectile", meta = (AllowPrivateAccess = true))
	TObjectPtr<USoundBase> ImpactSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoProjectile", meta = (AllowPrivateAccess = true))
	TObjectPtr<USoundBase> LoopingSound;
};
