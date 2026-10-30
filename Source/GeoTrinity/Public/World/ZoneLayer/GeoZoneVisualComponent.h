// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Components/SceneComponent.h"
#include "CoreMinimal.h"
#include "World/ZoneLayer/GeoZoneShape.h"

#include "GeoZoneVisualComponent.generated.h"

/**
 * One shape the zone layer draws (World/ZoneLayer/CLAUDE.md): data only, no mesh. Its owner sets what it is and in
 * which colours; the layer reads its world transform every tick, so attached to a moving or turning owner it follows
 * with no push. Shown while registered with UGeoZoneLayerSubsystem: from OnRegister, in game and editor worlds alike,
 * until SetZoneVisible(false) or OnUnregister.
 */
UCLASS(ClassGroup = (Geo))
class GEOTRINITY_API UGeoZoneVisualComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	/** Disables tick: the layer reads this component, it never pushes. */
	UGeoZoneVisualComponent();

	/** Registers the shape with the world's zone layer, unless SetZoneVisible(false) hid it. */
	virtual void OnRegister() override;
	/** Unregisters the shape from the world's zone layer. */
	virtual void OnUnregister() override;

	/** Shows or hides the shape at once: no fade. */
	void SetZoneVisible(bool bShow);

	/** Fills the shape off the server clock from StartServerTime to StartServerTime + Duration, in place of Fill. */
	void SetTimedFill(float StartServerTime, float Duration);

	/** Fill as drawn now: the timed fill when one is set, Fill otherwise. */
	float GetFill() const;

	/** Local-space size; see EGeoZoneShape. */
	FGeoZoneShape Shape;

	/** One bit per EGeoColor ordinal: GeoZoneLayer::MakeColorMask. */
	uint32 ColorMask = 0;

	EGeoZoneLayer Layer = EGeoZoneLayer::Ground;

	/** Covers nothing where a safe spot of MPC_MaskedArea shadows it: only the hazard owning the spots sets it. */
	bool bSafeSpotCut = false;

	/** 0..1, time spent of what the shape announces or lasts: the shade grows from the core to the outline. */
	float Fill = 0.f;

private:
	bool bZoneVisible = true;

	/** Server time SetTimedFill starts at. */
	float TimedFillStartServerTime = 0.f;
	/** 0 = no timed fill: Fill is drawn. */
	float TimedFillDuration = 0.f;
};
