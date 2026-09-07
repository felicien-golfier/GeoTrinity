// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"

#include "GeoFXComponent.generated.h"

class UAudioComponent;
class UGeoAbilitySystemComponent;
class UNiagaraComponent;
class UNiagaraSystem;
struct FGeoBuffFXEntry;
struct FGeoBurstFXMoment;
struct FGeoVFXParams;
struct FGeoSoundEntry;
struct FGeoSustainedFXMoment;

/** One sustained moment currently running on an owner: the system attached to it and the loop playing beside it. */
USTRUCT()
struct FGeoRunningSustainedFX
{
	GENERATED_BODY()

	/** The attached system, and what identifies the running moment — matched by asset against FGeoVFXParams::System. */
	UPROPERTY()
	TObjectPtr<UNiagaraComponent> VFXComponent;

	/** The moment's looping sound, null when it carries none or when it must not play on this machine. */
	UPROPERTY()
	TObjectPtr<UAudioComponent> AudioComponent;

	/** Takes both down. Niagara keeps simulating through a hidden actor, so a pooled owner has to go through this. */
	void Stop() const;
};

/**
 * Plays the authored VFX and sounds of whatever owns it. It knows *how* to play a moment, never *when* one happens:
 * each subclass owns its own moment set (EProjectileMoment, a character's buffs) and fires them, so no moment enum is
 * forced on every owner.
 *
 * Both kinds of playback live here — PlayBurst for a moment that fires and is gone, SetSustainedFX for one that is
 * turned on and back off — and both push their parameters through ApplyFXParams, the single place FGeoVFXParams reaches
 * a system.
 *
 * The one trigger it does own is the buff one, because both sides need the same one: it listens to every attribute of
 * UGameDataSettings::BuffFX on a source ASC and re-dresses the owner whenever a buff appears, changes or expires. Which
 * moment that shows is the subclass's answer (GetBuffMoment).
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
	 * Reads this owner's buff FX off SourceASC from now on: listens to every UGameDataSettings::BuffFX attribute so a
	 * buff appearing, changing or expiring re-dresses the owner, then shows whatever is already boosted.
	 *
	 * SourceASC is who the buffs belong to, not who wears them — a character passes its own, a projectile passes its
	 * shooter's, since a shot has no attributes of its own.
	 *
	 * Idempotent: call it again on the same ASC (a second InitGAS, a pooled shot refired by the same character) to
	 * re-evaluate without touching the bindings. Passing a different ASC clears the previous one first.
	 */
	void BindBuffFX(UGeoAbilitySystemComponent* SourceASC);

	/** Extra pitch factor laid over every sound this component plays, for an owner whose pitch follows something the
	 * sound entry itself cannot sample (AGeoShieldBurstProjectile's current radius). */
	void SetPitchMultiplier(float Multiplier);

	/** Spawns Moment's VFX at the owner's location, on the parameters Moment authored, and plays every sound it
	 * carries, all at once. Nothing is kept, so it is the wrong call for a moment that lasts.
	 * Public because an owner that is an actor rather than a component subclass (AGeoDeployableBase) fires its own
	 * moments from its own life cycle. */
	void PlayBurst(FGeoBurstFXMoment const& Moment) const;

protected:
	/**
	 * Turns Moment on or off on the owner: its system attached to the root, its sound looping beside it. An
	 * already-running moment is left running and only has its parameters re-pushed, so every path that can change what
	 * they resolve to just calls this again.
	 * A moment naming no VFX does nothing at all — the system is what identifies a running moment.
	 */
	void SetSustainedFX(FGeoSustainedFXMoment const& Moment, bool bShow);

	/** Pushes every parameter FGeoVFXParams carries onto Component, resolving what a curve reads against this owner. The
	 * one place they are written, burst or sustained: a new shared parameter is a field on the struct plus a line here.
	 */
	void ApplyFXParams(UNiagaraComponent* Component, FGeoVFXParams const& Params) const;

	/** Stops listening to the buff source and stops every sustained moment. Niagara keeps simulating through a hidden
	 * actor, so a pooled owner must be cleared on release or the next reuse renders the previous one's FX. */
	void ClearBuffFX();

	/** Entry's volume for this owner. Every sound this component plays goes through it, one-shot or looping. */
	float GetVolume(FGeoSoundEntry const& Entry) const;

	/** Entry's pitch for this owner, scaled by PitchMultiplier. Every sound this component plays goes through it. */
	float GetPitch(FGeoSoundEntry const& Entry) const;

	/** Who the feedback belongs to: sound audience gating, instigator-relative volume and every attribute a curve
	 * samples resolve against it. The owner itself unless a subclass answers otherwise. */
	virtual AActor* GetFXInstigator() const;

	/** Level the curves sample at when a sound entry or an FX moment reads from the ability level. */
	virtual int32 GetAbilityLevel() const;

	/** The moment Entry shows on this owner — its CharacterFX, worn by anything that carries its own attributes.
	 * Null when the entry has nothing to show here. */
	virtual FGeoSustainedFXMoment const* GetBuffMoment(FGeoBuffFXEntry const& Entry) const;

private:
	/** Matches the sustained moments to the attributes currently above their base value on BuffSourceASC. Leaves
	 * already-correct ones running, so every path that can change the answer just calls it. */
	void RefreshBuffFX();

	/** The project's buff catalog, empty while UGameDataSettings::BuffFX names no asset — the same "this game shows no
	 * buff FX" the empty catalog means. Bind, refresh and clear all walk it, so they all see the same list. */
	static TArray<FGeoBuffFXEntry> const& GetBuffEntries();

	/** Sustained moments currently running on the owner, matched by the asset of their system. */
	UPROPERTY()
	TArray<FGeoRunningSustainedFX> RunningSustainedFX;

	/** Whose buffs this owner shows. Weak: a projectile outliving its shooter must not keep that ASC alive. */
	TWeakObjectPtr<UGeoAbilitySystemComponent> BuffSourceASC;

	float PitchMultiplier = 1.f;
};
