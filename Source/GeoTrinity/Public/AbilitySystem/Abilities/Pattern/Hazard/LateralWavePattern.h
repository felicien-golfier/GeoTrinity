// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/AbilityPayload.h"
#include "AbilitySystem/Abilities/Pattern/Hazard/BeamPattern.h"
#include "AbilitySystem/Abilities/Pattern/Hazard/SafeSpot/SafeSpotSource.h"
#include "CoreMinimal.h"

#include "LateralWavePattern.generated.h"

class UMaterialParameterCollection;

/** What one firing of a lateral wave changes: its safe spots. Given by whoever fires it (GeoActivateAbilityAction). */
USTRUCT(BlueprintType)
struct GEOTRINITY_API FLateralWavePatternData : public FPatternData
{
	GENERATED_BODY()

	/** Safe spots of this firing, as offsets from the wave origin: X along the travel, Y across it. */
	UPROPERTY(EditAnywhere, Category = "GeoLateralWave")
	TArray<FVector2D> SafeSpotOffsets;

	/** True: the travel is one of 0 / 90 / 180 / 270 degrees rolled from the payload seed, starting at the arena edge
	 * opposite it and crossing the whole arena, instead of the pattern's own TravelYaw, origin, distance and width. */
	UPROPERTY(EditAnywhere, Category = "GeoLateralWave")
	bool bRandomDirection = false;
};

/**
 * Wave crossing the arena from the payload origin along TravelYaw, at Speed, over TravelDistance. The band
 * [front - BandWidth, front] (along the travel) hits everything within HalfWidth of the origin across the travel,
 * except what stands in a safe spot's lane (ESafeSpotShadow::Lane, parallel to the travel). Spots come from
 * SafeSpotSources, read every tick so a dead add drops its lane at once: Fixed from the firing's
 * FLateralWavePatternData, Adds from the caster's arena (server only, the tutorial is solo). The band is one beam
 * shape: during the wind-up a rectangle growing from the start edge along the travel until it covers the whole
 * travel, the moving band after; the zone layer cuts the lanes out of it from the safe spot mask.
 * A firing with FLateralWavePatternData::bRandomDirection rolls its direction and fits origin, distance and width to
 * the caster's arena.
 */
UCLASS(Blueprintable)
class GEOTRINITY_API ULateralWavePattern : public UBeamPattern
{
	GENERATED_BODY()

public:
	/** One of 0 / 90 / 180 / 270 degrees, the same for the same Seed on every machine. */
	static float RollTravelYaw(int32 Seed);

	/** This firing's travel direction in degrees (0 = +X); valid once InitPattern ran. */
	float GetFiringYaw() const { return FiringYaw; }
	/** How far this firing reaches each side of its origin, across the travel; valid once InitPattern ran. */
	float GetFiringHalfWidth() const { return FiringHalfWidth; }

protected:
	/** Sets this firing's travel (fixed, or rolled and fitted to the arena) before the base reads the hazard duration,
	 * and frees the safe spot mask a previous firing left. */
	virtual void InitPattern(FAbilityPayload const& Payload,
							 TInstancedStruct<FPatternData> const& PatternData) override;
	/** The front crosses TravelDistance, then the band leaves it. */
	virtual float GetHazardDuration() const override;
	/** True when Location lies in the band, across the wave's width, outside every safe spot's lane. */
	virtual bool IsInHazard(AActor const* Target, FVector2D Location, float SpentTime) const override;
	/** While SpentTime is negative, grows the beam from the start edge along the travel until it covers the whole
	 * travel when the wave starts; the band after. Uploads the safe spots to the mask and opts the shape into it
	 * (UGeoZoneVisualComponent::bSafeSpotCut). */
	virtual void MoveBeamVfx(float SpentTime) override;
	/** Frees the safe spot mask. */
	virtual void EndPattern(bool bForceStop = false) override;

private:
	/** Unit vector of the travel: FiringYaw, never the caster's facing (a miniboss turns while it walks). */
	FVector2D GetForward() const;

	/** Rolls FiringYaw from the payload seed, then starts the wave at the edge of the caster's arena opposite the
	 * travel and spans FiringTravelDistance / FiringHalfWidth over the arena along / across it. */
	void RollDirection(FAbilityPayload const& Payload);

	/** The safe spots as they stand now. */
	TArray<FSafeSpot> GetSafeSpots() const;

	/** Direction of the travel, in degrees (0 = +X). The caster's own facing plays no part. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoLateralWave", meta = (AllowPrivateAccess = "true"))
	float TravelYaw = 0.f;

	/** Centimetres per second the front crosses at. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoLateralWave", meta = (ClampMin = "1.0", AllowPrivateAccess = "true"))
	float Speed = 1500.f;

	/** How far the front goes from the origin, along the travel. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoLateralWave", meta = (ClampMin = "1.0", AllowPrivateAccess = "true"))
	float TravelDistance = 1400.f;

	/** Thickness of the band behind the front. TravelDistance leaves the floor lethal behind the front. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoLateralWave", meta = (ClampMin = "1.0", AllowPrivateAccess = "true"))
	float BandWidth = 250.f;

	/** The wave reaches this far each side of the origin, across the travel. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoLateralWave", meta = (ClampMin = "1.0", AllowPrivateAccess = "true"))
	float HalfWidth = 1200.f;

	/** Where the safe spots come from; several sources add up. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoLateralWave|SafeSpots", meta = (AllowPrivateAccess = "true"))
	TArray<ESafeSpotSource> SafeSpotSources = {ESafeSpotSource::Fixed, ESafeSpotSource::Adds};

	/** Radius of a spot: its lane is twice as wide. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoLateralWave|SafeSpots",
			  meta = (ClampMin = "1.0", AllowPrivateAccess = "true"))
	float SpotRadius = 150.f;

	/** MPC_MaskedArea — receives the safe spots, which the zone layer cuts out of the wave. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoLateralWave|VFX", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialParameterCollection> MaskMaterialParameterCollection;

	/** This firing's travel, set by InitPattern: Payload.Origin, TravelYaw, TravelDistance and HalfWidth, or the rolled
	 * direction fitted to the arena. Everything after InitPattern reads these, never the defaults. */
	FVector2D FiringOrigin = FVector2D::ZeroVector;
	float FiringYaw = 0.f;
	float FiringTravelDistance = 0.f;
	float FiringHalfWidth = 0.f;
};
