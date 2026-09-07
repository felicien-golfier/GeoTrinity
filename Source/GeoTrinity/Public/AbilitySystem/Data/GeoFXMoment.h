// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Data/GeoSoundRow.h"
#include "AttributeSet.h"
#include "CoreMinimal.h"
#include "Tool/GeoColor.h"

#include "GeoFXMoment.generated.h"

class UCurveFloat;
class UNiagaraSystem;

/**
 * A system and the User parameters it is played with. The visual half of a moment, grouped so a designer sets what the
 * moment looks like in one place, the way FGeoSoundEntry groups what it sounds like.
 * A parameter every system should carry is added here, once, and reaches projectile moments, deployable moments and
 * buff FX at the same time. UGeoFXComponent::ApplyFXParams is the one place that pushes them, so a new parameter is two
 * edits and no call site.
 *
 * A system that declares none of these parameters just ignores them.
 */
USTRUCT(BlueprintType)
struct FGeoVFXParams
{
	GENERATED_BODY()

	/** Spawned when the moment plays: at the owner's location for a burst, attached to it for a sustained one. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UNiagaraSystem> System;

	/** Pushed to GeoNiagaraParams::Color on every play. Always written, so a system reading it wears the moment's color
	 * rather than its own authored one. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGeoColorParam Color;

	/** Pushed to GeoNiagaraParams::RadiusOverride. 0 writes the SimpleCollisionRadius from the FX instigator. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0"))
	float RadiusOverride = 0.f;

	/** Instigator attribute MagnitudeCurve is sampled at. Hidden while bMagnitudeFromAbilityLevel is set — one source
	 * or the other, never both. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayAttribute MagnitudeAttribute;

	/** Samples MagnitudeCurve at the level the moment plays at instead of at an attribute value. Hidden while
	 * MagnitudeAttribute is set, for the same reason. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bMagnitudeFromAbilityLevel = false;

	/** Maps the chosen source — MagnitudeAttribute's value or the ability level — to the system's
	 * GeoNiagaraParams::NormalizedMagnitude, clamped to 0..1, so a system reads how strong this moment is without
	 * knowing what drives it. Hidden until one of the two sources is chosen, since it has nothing to sample against
	 * otherwise; the parameter is then written as 1, the full-strength value. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UCurveFloat> MagnitudeCurve;
};

/**
 * FGeoVFXParams for a system spawned to run itself out, which is the only kind that has a duration to be given.
 *
 * A leaf may only ever *add*: never hide or EditCondition a field declared in the base, since the engine resolves an
 * EditCondition operand only against the struct that declares the conditioned property, and an unresolved one silently
 * evaluates to true.
 */
USTRUCT(BlueprintType)
struct FGeoBurstVFXParams : public FGeoVFXParams
{
	GENERATED_BODY()

	/** Pushed to the system's GeoNiagaraParams::Lifetime. 0 writes nothing and leaves the system on its authored
	 * duration. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0"))
	float Lifetime = 0.f;
};

/**
 * A moment that fires and is gone: one spawn of its system at the owner's location plus every sound it carries, all at
 * once. Keyed by a per-owner moment enum (EProjectileMoment, EDeployableMoment) so a designer configures a moment's
 * whole feedback in one place instead of a sound map beside a separate VFX field.
 * Played through UGeoFXComponent::PlayBurst, which keeps nothing — the wrong struct for a moment that lasts.
 *
 * What the moment looks like and what it sounds like are each grouped in their own struct; anything a future parameter
 * has to say about both belongs beside them, at this level.
 */
USTRUCT(BlueprintType)
struct FGeoBurstFXMoment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGeoBurstVFXParams VFX;

	/** Every sound of the moment fires together, so it can layer several assets over the one system. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FGeoSoundEntry> Sounds;
};

/**
 * A moment that is turned on and back off again: its system stays attached to the owner and its sound loops for as long
 * as it runs — a buff worn while an attribute is boosted, a shot's whole flight.
 * Played through UGeoFXComponent::SetSustainedFX, which keeps the running instance and re-pushes the VFX parameters
 * whenever what drives them moves.
 *
 * A sustained moment naming no system shows nothing at all: the system is what identifies a running moment, so there is
 * nothing for its sound to loop against.
 */
USTRUCT(BlueprintType)
struct FGeoSustainedFXMoment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGeoVFXParams VFX;

	/** The one sound looping while the moment runs. One and not a list because a loop has to be stopped again: it plays
	 * on a single audio component held for exactly this moment. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGeoSoundEntry Sound;
};
