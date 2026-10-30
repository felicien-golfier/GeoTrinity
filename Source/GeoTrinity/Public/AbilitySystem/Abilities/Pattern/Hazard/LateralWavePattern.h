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
};

/**
 * Wave crossing the arena from the payload origin along TravelYaw, at Speed, over TravelDistance. The band
 * [front - BandWidth, front] (along the travel) hits everything within HalfWidth of the origin across the travel,
 * except what stands in a safe spot's lane (ESafeSpotShadow::Lane, parallel to the travel). Spots come from
 * SafeSpotSources, read every tick so a dead add drops its lane at once: Fixed from the firing's
 * FLateralWavePatternData, Adds from the caster's arena (server only, the tutorial is solo). The band is one beam
 * shape: the whole travel during the wind-up, the moving band after; the zone layer cuts the lanes out of it from the
 * safe spot mask.
 */
UCLASS(Blueprintable)
class GEOTRINITY_API ULateralWavePattern : public UBeamPattern
{
	GENERATED_BODY()

protected:
	/** Frees the safe spot mask a previous firing left. */
	virtual void InitPattern(FAbilityPayload const& Payload,
							 TInstancedStruct<FPatternData> const& PatternData) override;
	/** The front crosses TravelDistance, then the band leaves it. */
	virtual float GetHazardDuration() const override;
	/** True when Location lies in the band, across the wave's width, outside every safe spot's lane. */
	virtual bool IsInHazard(AActor const* Target, FVector2D Location, float SpentTime) const override;
	/** Lays the beam over the whole travel while SpentTime is negative, on the band after, uploads the safe spots to
	 * the mask and opts the shape into it (UGeoZoneVisualComponent::bSafeSpotCut). */
	virtual void MoveBeamVfx(float SpentTime) override;
	/** Frees the safe spot mask. */
	virtual void EndPattern(bool bForceStop = false) override;

private:
	/** Unit vector of the travel: TravelYaw, never the caster's facing (a miniboss turns while it walks). */
	FVector2D GetForward() const;

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
};
