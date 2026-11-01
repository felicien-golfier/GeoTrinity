// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Pattern/Hazard/SafeSpot/SafeSpot.h"
#include "AbilitySystem/Abilities/Pattern/Pattern.h"
#include "CoreMinimal.h"
#include "Tool/Visual/GeoColor.h"
#include "World/ZoneLayer/GeoZoneShape.h"

#include "ZoneHazardPattern.generated.h"

class UGeoZoneVisualComponent;
class UMaterialParameterCollection;

/**
 * Hazard drawn on the zone layer as one shape: a telegraph filling over the wind-up, then the live hazard on the
 * ground filling over its duration, hidden when it ends. A child only lays the shape out (MoveHazardVisual); safe
 * spots cut out of it go through SetSafeSpots.
 */
UCLASS(Abstract)
class GEOTRINITY_API UZoneHazardPattern : public UPattern
{
	GENERATED_BODY()

public:
	/** Turns the hazard on. */
	UZoneHazardPattern();

protected:
	/** Creates the hidden shape on rendering machines; the pattern instance and its shape are reused per activation.
	 */
	virtual void OnCreate(FGameplayTag AbilityTag, AActor& Owner) override;

	/** Frees the safe spot mask, then shows the shape as a telegraph filling over the wind-up, placed by
	 * MoveHazardVisual before it is shown so its first frame is already in place. No telegraph on the "too late" path
	 * (no wind-up timer). */
	virtual void InitPattern(FAbilityPayload const& Payload,
							 TInstancedStruct<FPatternData> const& PatternData) override;
	/** Turns the telegraph into the live hazard, on the ground layer, filling over the hazard's duration; placed and
	 * shown. */
	virtual void StartPattern() override;
	/** Keeps the telegraph placed while the wind-up runs. */
	virtual void TickDuringInit(float SpentTime) override;
	/** Keeps the live hazard placed. */
	virtual void TickPattern(float ServerTime, float SpentTime) override;
	/** Hides the shape. */
	virtual void OnHazardEnd() override;
	/** Hides the shape, still up when force-stopped, and frees the safe spot mask. */
	virtual void EndPattern(bool bForceStop = false) override;

	/** Sets the shape, size and place of HazardVisual at SpentTime (negative in the wind-up). Does nothing here. */
	virtual void MoveHazardVisual(float SpentTime);

	/** Uploads Spots to the mask (FSafeSpotMask::Set) and opts the shape into it. */
	void SetSafeSpots(TArray<FSafeSpot> const& Spots, FVector2D LaneDirection, FVector2D Origin);
	/** Frees every mask slot. */
	void ClearSafeSpots();

	/** Colour of the hazard and of its telegraph, and of their outlines. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoZoneHazard|GameFeel")
	FGeoColorParam HazardColor;

	/** Further colours the hazard and its telegraph draw beside HazardColor, one per extra meaning the hazard carries,
	 * mixed by the zone layer. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoZoneHazard|GameFeel")
	TArray<FGeoColorParam> SecondaryHazardColors;

	/** Where the wind-up telegraph's fill grows from (hex beam: its middle line; tutorial wave: along its travel). */
	UPROPERTY(EditDefaultsOnly, Category = "GeoZoneHazard|GameFeel")
	EGeoFillMode TelegraphFillMode = EGeoFillMode::FromCore;

	/** Where the live hazard's fill grows from. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoZoneHazard|GameFeel")
	EGeoFillMode HazardFillMode = EGeoFillMode::FromCore;

	/** MPC_MaskedArea — receives the safe spots, which the zone layer cuts out of the hazard. Unset for a hazard with
	 * no spots. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoZoneHazard|VFX")
	TObjectPtr<UMaterialParameterCollection> MaskMaterialParameterCollection;

	/** The hazard on the zone layer, placed in world space by MoveHazardVisual; null on a dedicated server. */
	UPROPERTY(Transient)
	TObjectPtr<UGeoZoneVisualComponent> HazardVisual;
};
