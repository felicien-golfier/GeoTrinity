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
 * Everything one moment of an actor's life plays: its Niagara system, the User parameters that system runs with, and
 * its sounds. Keyed by a per-actor moment enum (EProjectileMoment, EDeployableSoundType) so a designer configures a
 * moment's whole feedback in one place instead of a sound map beside a separate VFX field.
 * Every sound of a moment fires together, so a moment can layer several assets over the one system.
 * Played through UGeoFXComponent, which resolves the audience, volume and pitch rules and pushes the parameters below
 * onto the spawned system. A system that declares none of them just ignores them.
 */
USTRUCT(BlueprintType)
struct FGeoFXMoment
{
	GENERATED_BODY()

	/** Spawned at the owner when the moment fires — attached to it for a moment that lasts (EProjectileMoment::Looping),
	 * one-shot at its location otherwise. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UNiagaraSystem> VFX;

	/** Pushed to the system's particle color parameter (GeoNiagaraParams::Color) on every play. Always written, so a
	 * system reading it wears the moment's color rather than its own authored one. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGeoColorParam Color;

	/** Pushed to the system's GeoNiagaraParams::Radius. 0 writes nothing and leaves the system on its authored size. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0"))
	float Radius = 0.f;

	/** Pushed to the system's GeoNiagaraParams::Lifetime. 0 writes nothing and leaves the system on its authored
	 * duration. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0"))
	float Lifetime = 0.f;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FGeoSoundEntry> Sounds;
};
