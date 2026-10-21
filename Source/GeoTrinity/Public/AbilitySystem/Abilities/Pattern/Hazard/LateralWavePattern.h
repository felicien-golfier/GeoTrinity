// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Pattern/Hazard/BeamPattern.h"
#include "AbilitySystem/Abilities/Pattern/Hazard/SafeSpot/SafeSpotSource.h"
#include "CoreMinimal.h"

class UMaterialParameterCollection;

#include "LateralWavePattern.generated.h"

/**
 * Wave crossing the arena from the payload origin along TravelYaw, at Speed, over TravelDistance. The band
 * [front - BandWidth, front] (along the travel) hits everything within HalfWidth of the origin across the travel,
 * except what stands in a safe spot's lane (ESafeSpotShadow::Lane, parallel to the travel). A BandWidth of
 * TravelDistance fills the floor behind the front instead (nothing to dodge but the lanes). Spots come from
 * SafeSpotSources, read every tick so a dead add drops its lane at once; the arena keeps its adds on the server only,
 * so a client sees no add lane: the tutorial is solo. The beam VFX draws the whole rectangle during the wind-up and the
 * band after; the beam material cuts the lanes out of both through the mask MPC.
 */
UCLASS(Blueprintable)
class GEOTRINITY_API ULateralWavePattern : public UBeamPattern
{
	GENERATED_BODY()

protected:
	/** Frees the mask MPC of a previous wave's lanes. */
	virtual void InitPattern(FAbilityPayload const& Payload,
							 TInstancedStruct<FPatternData> const& PatternData) override;
	/** Keeps the telegraph on the lanes as they stand. */
	virtual void TickDuringInit(float SpentTime) override;
	/** Moves the band and keeps the mask on the lanes as they stand. */
	virtual void TickPattern(float ServerTime, float SpentTime) override;
	/** The front crosses TravelDistance, then the band leaves it. */
	virtual float GetHazardDuration() const override;
	/** True when Location lies in the band, across the wave's width, outside every safe spot's lane. */
	virtual bool IsInHazard(AActor const* Target, FVector2D Location, float SpentTime) const override;
	/** Lays the beam VFX on the whole rectangle while SpentTime is negative, on the band after. */
	virtual void MoveBeamVfx(float SpentTime) override;
	/** Frees the mask MPC. */
	virtual void EndPattern(bool bForceStop = false) override;

private:
	/** Unit vector of the travel: TravelYaw, never the caster's facing (a miniboss turns while it walks). */
	FVector2D GetForward() const;

	/** The safe spots as they stand now. */
	TArray<FSafeSpot> GetSafeSpots() const;

	/** Uploads the safe spots as they stand now to the mask MPC. */
	void RefreshMask();

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
	TArray<ESafeSpotSource> SafeSpotSources;

	/** Fixed spots, as offsets from the origin: X along the travel, Y across it. Used with the Fixed source. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoLateralWave|SafeSpots", meta = (AllowPrivateAccess = "true"))
	TArray<FVector2D> FixedSpotOffsets;

	/** Radius of an Adds or Fixed spot: its lane is twice as wide. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoLateralWave|SafeSpots",
			  meta = (ClampMin = "1.0", AllowPrivateAccess = "true"))
	float SpotRadius = 150.f;

	/** MPC_MaskedArea — receives the spots to cut their lanes out of the beam material. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoLateralWave|SafeSpots", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialParameterCollection> MaskMaterialParameterCollection;
};
