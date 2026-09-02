// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/AbilityPayload.h"
#include "AbilitySystem/Data/GeoSoundRow.h"
#include "Actor/Projectile/ExternalProjectileParams.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StructUtils/InstancedStruct.h"

#include "GeoProjectile.generated.h"

class AGeoCharacter;
class UGeoAbilitySystemComponent;
class UGeoGameFeelComponent;
class UProjectileMovementComponent;
class UNiagaraSystem;
class UNiagaraComponent;
class USphereComponent;
class USceneComponent;
class UAudioComponent;
class UPrimitiveComponent;
struct FHitResult;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectileEndLife, AGeoProjectile*, Projectile);

/**
 * Replicable, effect-applying projectile that terminates on distance-span, lifespan, or a valid hit.
 * Base class for all game projectiles — extend AGeoPooledProjectile for pool-managed variants.
 */
UCLASS()
class GEOTRINITY_API AGeoProjectile : public AActor
{
	GENERATED_BODY()
public:
	/** Creates the sphere collider (root), projectile movement component, and looping audio component as
	 *  default subobjects; enables replication and tick. */
	AGeoProjectile();
	/** Seeds ResolvedParams with DefaultParams, so a projectile spawned outside GeoASLib (never running
	 *  ApplyProjectileParams) still runs on its Blueprint values. */
	virtual void PostInitProperties() override;
	/** Registers PredictionKeyId for replication. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	/**
	 * For non-pooled projectiles: re-applies movement on the server (Blueprint construction resets velocity),
	 * and calls InitProjectileLife on clients. Also destroys the matching predicted projectile on the owning client
	 * when CVarReplaceLocalProjectiles is enabled.
	 */
	virtual void BeginPlay() override;
	/** Guards against double-ending by checking bIsEnding before calling EndProjectileLife. */
	virtual void LifeSpanExpired() override;
	/** Plays the impact FX on non-authority machines that did not already end locally (bIsEnding) — replicated
	 * destruction never runs EndProjectileLife there. */
	virtual void Destroyed() override;
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

	/**
	 * Overrides the travel speed for this projectile instance.
	 *
	 * @param Speed  Movement speed in cm/s applied to the projectile movement component.
	 */
	UFUNCTION(BlueprintCallable)
	void OverrideSpeed(float Speed);

	/**
	 * Cosmetic only: draws the bullet from WorldLocation instead of the actor, then slides it back onto the actor over
	 * the first moments of flight (Tick). Collision, travel distance and impact all keep running from the actor's own
	 * spawn point, so a caller can show a shot leaving somewhere else (a deploy leaving its satellite ring) without
	 * moving anything the game reads.
	 *
	 * @param WorldLocation  World point the visual starts from.
	 */
	void SetVisualLaunchLocation(FVector const& WorldLocation);

	/**
	 * Applies a spawn-params bundle to this instance. Each value resolves per its EOverrideParam toggle —
	 * GameDataSettings value, the projectile's DefaultParams, or explicit override — and the resolved bundle is pushed
	 * via ApplyParams. A reused pooled projectile is fully re-resolved each spawn (DefaultParams is never written to),
	 * so it never keeps the previous instance's values.
	 */
	void ApplyProjectileParams(FExternalProjectileParams const& Params);

#if WITH_EDITOR
	/** Previews a DefaultParams edit without entering play. A Class Defaults edit lands on the CDO, whose BulletVFX is
	 * an unregistered template nothing renders, so it forwards the preview to the live instances (Blueprint preview
	 * actor, placed actors) instead. */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

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

	/** Resolved travel speed replicated from the server so simulated proxies move at the same speed as authority
	 * (server-authoritative projectiles never ran ApplyProjectileParams client-side). 0 = unset, keep the movement
	 * component's own InitialSpeed. */
	UPROPERTY(Replicated)
	float ReplicatedSpeed = 0.f;

	FOnProjectileEndLife OnProjectileEndLifeDelegate;

protected:
	/**
	 * Returns true when OtherActor is a valid hit target for this projectile.
	 * Default implementation checks team attitude bitmask. Override to restrict targeting (e.g. ground only).
	 * On success OutOwnerASC / OutTargetASC hold the ASCs it already had to resolve, so HandleValidOverlap does not
	 * look them up a second time.
	 */
	virtual bool IsValidOverlap(AActor* OtherActor, UGeoAbilitySystemComponent*& OutOwnerASC,
								UGeoAbilitySystemComponent*& OutTargetASC);

	/**
	 * Called from OnSphereOverlap after IsValidOverlap passes, with the ASCs it resolved. Override to customise hit
	 * behaviour. Default: applies EffectDataArray to target, calls OnProjectileHit, ends projectile life.
	 */
	virtual void HandleValidOverlap(AActor* OtherActor, UGeoAbilitySystemComponent* OwnerASC,
									UGeoAbilitySystemComponent* TargetASC);

	/**
	 * Runs the hit path against everything the projectile already sits inside, for a shot fired point-blank into a
	 * hitbox. The pool enables collision — and broadcasts that first begin-overlap — before InitProjectileLife binds the
	 * delegate, so nothing is listening when it fires and the engine never repeats it while the projectile stays inside.
	 * Deferred to the next tick by InitProjectileLife so a spawn-frame hit cannot recycle the projectile under the
	 * ability that is still spawning it.
	 */
	void ResolveInitialOverlaps();

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

	/** Plays EndSoundType (ValidOverlapEnd after a valid overlap, NoOverlapEnd otherwise) and spawns ImpactEffect
	 * (Niagara) at the actor's current location. */
	UFUNCTION(BlueprintCallable)
	virtual void PlayImpactFx() const;

	/**
	 * Called on every machine that has this projectile when it hits a valid actor.
	 * Override in Blueprint to apply a hit flash or other cosmetic reaction on the HitActor's mesh.
	 *
	 * @param HitActor  The actor that was struck. Cast to GeoCharacter to access its mesh.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "GeoProjectile|GameFeel")
	void OnProjectileHit(AActor* HitActor);

	/**
	 * Returns the pitch multiplier for the sound identified by SoundType.
	 * Default: looks up ResolvedParams.SoundMap and forwards to GetPitch(Entry).
	 * Returns 1.0 if no entry exists. Override in Blueprint for custom logic.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "GeoProjectile|Audio")
	float GetPitch(EProjectileSoundType SoundType) const;

	/** Returns the pitch multiplier for Entry via UGeoSoundRowLibrary::GetPitch with the projectile's instigator.
	 * Virtual so subclasses can layer additional pitch factors (e.g. size). */
	virtual float GetPitch(FGeoSoundEntry const& Entry) const;

	/** Plays the mapped sound once at the actor's location with its audience-gated volume and attribute-driven pitch.
	 */
	void PlaySoundOneShot(EProjectileSoundType SoundType) const;

	/** Plays Entry once at the actor's location with its audience-gated volume and attribute-driven pitch. */
	void PlaySoundOneShot(FGeoSoundEntry const& Entry) const;

	/** Returns Payload.Instigator, falling back to GetInstigator() when the payload is not replicated. */
	AActor* GetCurrentInstigator() const;

	/** Called when the projectile's life ends (distance exceeded, lifespan expired, or valid hit). Destroys the actor
	 * on authority (including client-predicted fakes); simulated proxies only go dark and wait for the server's
	 * replicated destruction, so a local Destroy() can never race a later replication bunch into a ghost re-spawn. */
	virtual void EndProjectileLife();
	/** Removes the OnRevived binding made in InitProjectileLife. Called from EndProjectileLife (non-pooled) and
	 * AGeoPooledProjectile::End (pool release) so a reused projectile never keeps a binding to a previous instigator.
	 */
	void UnbindFromInstigatorRevive();
	/** Configures the UProjectileMovementComponent from the projectile's UPROPERTY settings. */
	void InitProjectileMovementComponent();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USphereComponent> Sphere;

	UPROPERTY()
	TObjectPtr<UAudioComponent> LoopingSoundComponent;

	/** Bullet visual. Native subobject (its system asset is set in the constructor; the Blueprint may override it). Its
	 * User.* params are write-only from C++ — ApplyParams pushes radius/colors/trail; nothing reads them back, which is
	 * why the values live in DefaultParams rather than in the Niagara asset's own user-param defaults. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GeoProjectile")
	TObjectPtr<UNiagaraComponent> BulletVFX;

	/** Dresses the shot in whatever buff its owner is carrying (UGameDataSettings::BuffVFX). InitProjectileLife points
	 * it at Payload.Owner's ASC — a shot has no attributes of its own. Native subobject so no projectile Blueprint can
	 * be missing it. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GeoProjectile")
	TObjectPtr<UGeoGameFeelComponent> GameFeelComponent;

	/** Per-Blueprint default values, edited in Class Defaults. Resolved against for KeepBlueprintDefaultValue, seeds
	 * ResolvedParams, and is never written to at runtime. Editing it previews live on any instance in an editor
	 * viewport (see PostEditChangeProperty). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoProjectile|Params")
	FProjectileParamsBase DefaultParams;

	/** The values this instance is actually running with: DefaultParams until a spawn resolves its
	 * FExternalProjectileParams over them (ApplyParams). Read it — never DefaultParams — for runtime behaviour. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "GeoProjectile|Params")
	FProjectileParamsBase ResolvedParams;

	EProjectileSoundType EndSoundType = EProjectileSoundType::NoOverlapEnd;

	bool bIsEnding{false};

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoProjectile",
			  meta = (Tooltip = "Safe guard in case distance check fails", AllowPrivateAccess = true))
	float LifeSpanInSec = 30.f;


	/** Ends the projectile when its instigating GeoCharacter revives. Bound in InitProjectileLife, unbound in
	 * EndProjectileLife so pooled reuse never keeps a binding to a previous instigator. */
	void OnInstigatorRevived();

	TWeakObjectPtr<AGeoCharacter> ReviveBoundInstigator;
	FDelegateHandle InstigatorRevivedHandle;

	FVector InitialPosition;
	float DistanceSpanSqr;

	/** Stores Params as ResolvedParams, pushes its radius/colors/trail onto BulletVFX and resizes the sphere collider
	 * to match. Single write path shared by ApplyProjectileParams (resolved values), the simulated-proxy default apply,
	 * and the editor preview. */
	void ApplyParams(FProjectileParamsBase const& Params);

#if WITH_EDITOR
	/** Applies DefaultParams to this instance and restarts its bullet system so the new values are on screen. */
	void PreviewParams();
#endif

	/** Cosmetic (let the juice flow) **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoProjectile", meta = (AllowPrivateAccess = true))
	TObjectPtr<UNiagaraSystem> ImpactEffect;
};
