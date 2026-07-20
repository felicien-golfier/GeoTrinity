// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Pattern/Pattern.h"
#include "CoreMinimal.h"

#include "TileBombPattern.generated.h"

/**
 * Pattern data for UTileBombPattern: the player the bomb rides, drawn once on the server by UGeoTileBombAbility so
 * every machine shows the countdown on the same player.
 */
USTRUCT()
struct FTileBombPatternData : public FPatternData
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AActor> BombCarrier;
};

/**
 * Bomb stuck to one player for the whole wind-up, then detonated wherever that player is standing when it goes off:
 * every tile within BlastRadius is destroyed and the ability's effect data applied to whoever is caught in the blast.
 * The carrier decides where the hole ends up — walk it out to the rim, or lose the middle of the platform.
 */
UCLASS(Blueprintable)
class GEOTRINITY_API UTileBombPattern : public UPattern
{
	GENERATED_BODY()

protected:
	/** Reads the carrier out of the pattern data before Super fires the countdown cue at them. */
	virtual void InitPattern(FAbilityPayload const& Payload, TInstancedStruct<FPatternData> const& PatternData) override;
	/** Server. Applies the effect data around the carrier, carves the tiles under them, then ends. */
	virtual void StartPattern() override;
	/** Points the cue at the carrier so the countdown visual can ride them. */
	virtual FGameplayCueParameters FillCueParam(FAbilityPayload const& Payload) override;

	/** Radius of the blast, in world units: both the tiles destroyed and the actors the effect data reaches. */
	UPROPERTY(EditDefaultsOnly, Category = "Bomb", meta = (ClampMin = "0.0"))
	float BlastRadius = 250.f;

private:
	UPROPERTY(Transient)
	TObjectPtr<AActor> BombCarrier;
};
