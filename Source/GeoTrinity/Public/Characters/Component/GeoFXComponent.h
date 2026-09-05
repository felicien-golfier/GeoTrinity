// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"

#include "GeoFXComponent.generated.h"

class UGeoAbilitySystemComponent;
class UNiagaraComponent;
class UNiagaraSystem;
struct FGeoBuffVFXEntry;
struct FGeoFXMoment;
struct FGeoSoundEntry;

/**
 * Plays the authored VFX and sounds of whatever owns it. It knows *how* to play a moment, never *when* one happens:
 * each subclass owns its own moment set (EProjectileMoment, a character's buffs) and fires them, so no moment enum is
 * forced on every owner.
 *
 * The one exception it does own is the buff trigger, because both sides need the same one: it listens to every
 * UGameDataSettings::BuffVFX attribute on a source ASC and re-dresses the owner whenever a buff appears or expires.
 * Which system that shows is the subclass's answer (GetBuffVFXSystem).
 *
 * Every path is silent on a dedicated server.
 */
UCLASS(Abstract, ClassGroup = "GeoTrinity")
class GEOTRINITY_API UGeoFXComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Plays Entry once at the owner's location, audience-gated, with this component's volume and pitch. */
	void PlaySound(FGeoSoundEntry const& Entry) const;

	/**
	 * Reads this owner's buff VFX off SourceASC from now on: listens to every UGameDataSettings::BuffVFX attribute so a
	 * buff appearing or expiring re-dresses the owner, then shows whatever is already boosted.
	 *
	 * SourceASC is who the buffs belong to, not who wears them — a character passes its own, a projectile passes its
	 * shooter's, since a shot has no attributes of its own.
	 *
	 * Idempotent: call it again on the same ASC (a second InitGAS, a pooled shot refired by the same character) to
	 * re-evaluate without touching the bindings. Passing a different ASC clears the previous one first.
	 */
	void BindBuffVFX(UGeoAbilitySystemComponent* SourceASC);

	/** Extra pitch factor laid over every sound this component plays, for an owner whose pitch follows something the
	 * sound entry itself cannot sample (AGeoShieldBurstProjectile's current radius). */
	void SetPitchMultiplier(float Multiplier);

protected:
	/** Spawns Moment's VFX at the owner's location and plays every sound it carries, all at once. One-shot: nothing is
	 * kept, so it is the wrong call for a moment that lasts. */
	void PlayMoment(FGeoFXMoment const& Moment) const;

	/** Stops listening to the buff source and destroys every attached system. Niagara keeps simulating through a hidden
	 * actor, so a pooled owner must be cleared on release or the next reuse renders the previous one's VFX. */
	void ClearBuffVFX();

	/** Entry's volume for this owner. Every sound this component plays goes through it, one-shot or looping. */
	float GetVolume(FGeoSoundEntry const& Entry) const;

	/** Entry's pitch for this owner, scaled by PitchMultiplier. Every sound this component plays goes through it. */
	float GetPitch(FGeoSoundEntry const& Entry) const;

	/** Who the sounds belong to: audience gating and instigator-relative volume are resolved against it. The owner
	 * itself unless a subclass answers otherwise. */
	virtual AActor* GetSoundInstigator() const;

	/** Level the sound curves sample at when an entry reads from the ability level. */
	virtual int32 GetAbilityLevel() const;

	/** The system Entry shows on this owner — its CharacterVFX, worn by anything that carries its own attributes.
	 * Null when the entry has nothing to show here. */
	virtual UNiagaraSystem* GetBuffVFXSystem(FGeoBuffVFXEntry const& Entry) const;

private:
	/** Matches the attached VFX to the attributes currently above their base value on BuffSourceASC. Leaves
	 * already-correct systems running, so every path that can change the answer just calls it. */
	void RefreshBuffVFX();

	/** Adds or removes System as a system attached to the owner's root, leaving an already-correct one running.
	 * Matched by asset, since that is what a buff entry resolves to. */
	void SetAttachedVFX(UNiagaraSystem* System, bool bShow);

	/** Systems currently attached to the owner, matched by asset. */
	UPROPERTY()
	TArray<TObjectPtr<UNiagaraComponent>> AttachedVFXComponents;

	/** Whose buffs this owner shows. Weak: a projectile outliving its shooter must not keep that ASC alive. */
	TWeakObjectPtr<UGeoAbilitySystemComponent> BuffSourceASC;

	float PitchMultiplier = 1.f;
};
