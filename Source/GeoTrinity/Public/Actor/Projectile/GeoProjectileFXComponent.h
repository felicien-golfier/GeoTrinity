// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Characters/Component/GeoFXComponent.h"
#include "CoreMinimal.h"

#include "GeoProjectileFXComponent.generated.h"

enum class EProjectileMoment : uint8;
class UAudioComponent;
class UNiagaraComponent;

/**
 * Everything a shot looks and sounds like, in one place: the bullet visual, the looping audio, and every moment of
 * FProjectileParamsBase::FXMap. The projectile itself keeps no FX of its own — it names the moments it reaches
 * (StartLife, PlayEnd) and this decides what they play.
 *
 * It owns neither subobject it drives: the bullet visual and the looping audio are the projectile's, handed over in its
 * constructor, so a projectile Blueprint keeps editing them where it always has.
 *
 * Sounds resolve their audience, volume and pitch against the shot's *shooter*, not the shot — a projectile has no
 * attributes of its own, and a shot fired by the local player must sound like their own shot.
 */
UCLASS(ClassGroup = "GeoTrinity")
class GEOTRINITY_API UGeoProjectileFXComponent : public UGeoFXComponent
{
	GENERATED_BODY()

public:
	/** Enables tick for the visual launch offset only; every other path is event-driven. */
	UGeoProjectileFXComponent();

	/** Slides the bullet visual back onto the actor after SetVisualLaunchLocation drew it elsewhere, then turns itself
	 * off. */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;

	/** Adopts the projectile's own bullet visual and looping audio subobjects. Called from AGeoProjectile's
	 * constructor — both are needed before BeginPlay, since a spawn resolves its params on a projectile that has not
	 * begun play yet. */
	void SetPlaybackSubobjects(UNiagaraComponent* InBulletVFX, UAudioComponent* InLoopingSound);

	/**
	 * Pushes the owner's resolved radius, colors and trail lifetime onto the bullet visual, and swaps its system for
	 * the Looping moment's VFX when that moment names one — which is how a spawn site re-skins the bullet through
	 * FExternalProjectileParams. Leaves the Blueprint's own system in place otherwise.
	 * Call it from AGeoProjectile::ApplyParams only, which stores ResolvedParams first.
	 */
	void ApplyParams();

	/** Restarts the bullet visual on its current params, plays the Start moment and starts the Looping one. Called on
	 * every spawn, a pooled reuse included — which is why the visual is re-activated here rather than left running:
	 * hiding a pooled actor does not stop a Niagara system. */
	void StartLife() const;

	/** Plays the moment the shot ended on. A valid overlap layers ValidOverlapEnd *over* NoOverlapEnd, so a shot that
	 * connects still gets the plain impact plus whatever the hit adds. */
	void PlayEnd(bool bValidOverlap) const;

	/** Takes down everything still running: the attached buff VFX, the looping sound, and the bullet visual. */
	void StopAll();

	/**
	 * Cosmetic only: draws the bullet from WorldLocation instead of the actor, then slides it back on over the first
	 * moments of flight. Collision, travel distance and impact all keep running from the actor's own spawn point.
	 *
	 * @param WorldLocation  World point the visual starts from.
	 */
	void SetVisualLaunchLocation(FVector const& WorldLocation);

	/** Sets the bullet visual's radius on its own, for a shot whose size changes mid-flight
	 * (AGeoShieldBurstProjectile grows on every bounce). */
	void SetBulletRadius(float Radius) const;

protected:
	/** The shot's shooter — a projectile carries no attributes and is never the local player's avatar itself. */
	virtual AActor* GetSoundInstigator() const override;

	/** The level the shot was fired at, off its payload. */
	virtual int32 GetAbilityLevel() const override;

	/**
	 * A shot only carries the two boosts it can actually express — damage and applied heal — and only when it carries
	 * the effect that attribute scales, so a damage buff never lights up a heal shot. Every other attribute shows on
	 * the shooter alone, and returns null here.
	 */
	virtual UNiagaraSystem* GetBuffVFXSystem(FGeoBuffVFXEntry const& Entry) const override;

private:
	/** The moment Type of the owning projectile's resolved FX map, or null when it holds none. */
	FGeoFXMoment const* FindMoment(EProjectileMoment Type) const;

	/** Points the bullet visual at the Looping moment's VFX, or back at the Blueprint's own system when this spawn
	 * names none — a pooled projectile is re-resolved every reuse and must not keep the previous shot's skin. */
	void ApplyBulletSystem();

	/** The projectile's bullet visual — its User.* params are write-only from here; nothing reads them back. */
	UPROPERTY()
	TObjectPtr<UNiagaraComponent> BulletVFX;

	/** The system the projectile Blueprint authored on BulletVFX, captured on the first spawn — before any Looping
	 * moment has replaced it — so it can be restored. */
	UPROPERTY()
	TObjectPtr<UNiagaraSystem> DefaultBulletSystem;

	/** The projectile's one looping audio component, which is why the Looping moment plays a single sound. */
	UPROPERTY()
	TObjectPtr<UAudioComponent> LoopingSoundComponent;
};
