// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Data/GeoSoundRow.h"
#include "CoreMinimal.h"

#include "GeoFXMoment.generated.h"

class UNiagaraSystem;

/**
 * Everything one moment of an actor's life plays: its Niagara system and its sounds. Keyed by a per-actor moment enum
 * (EProjectileMoment, EDeployableSoundType) so a designer configures a moment's whole feedback in one place instead of
 * a sound map beside a separate VFX field.
 * Every sound of a moment fires together, so a moment can layer several assets over the one system.
 * Played through UGeoFXComponent, which resolves the audience, volume and pitch rules.
 */
USTRUCT(BlueprintType)
struct FGeoFXMoment
{
	GENERATED_BODY()

	/** Spawned at the owner when the moment fires — attached to it for a moment that lasts (EProjectileMoment::Looping),
	 * one-shot at its location otherwise. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UNiagaraSystem> VFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FGeoSoundEntry> Sounds;
};
