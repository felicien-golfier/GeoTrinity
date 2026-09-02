// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Pattern/BeamPattern.h"
#include "CoreMinimal.h"

#include "TileCarveRayPattern.generated.h"

class AGeoHexArena;

/**
 * Beam that carves the hex platform it is fired over: it telegraphs the furthest still-standing tile it reaches while
 * firing, then destroys that tile on its way out — the tank picks which rim tile that is by choosing where they stand
 * when the boss locks on. Keeps the hex arena out of UBeamPattern, which any arena can fire.
 */
UCLASS(Blueprintable)
class GEOTRINITY_API UTileCarveRayPattern : public UBeamPattern
{
	GENERATED_BODY()

protected:
	/** Highlights the tile the beam is about to carve, then runs the regular beam tick. */
	virtual void TickPattern(float ServerTime, float SpentTime) override;
	/** Destroys the tile the beam locked onto when it fired, unless the pattern was force-stopped. */
	virtual void EndPattern(bool bForceStop = false) override;

	/** Destroys the furthest arena tile the beam reaches, at the moment it fires. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoBeam")
	bool bDestroyLastTileHit = true;

private:
	/**
	 * Returns the hex arena of the boss firing this beam, and the furthest still-standing tile the beam reaches at
	 * SpentTime. Null when the beam crosses no living tile.
	 *
	 * @param OutTile  Set to the furthest alive tile along the beam whenever an arena is returned.
	 */
	AGeoHexArena* FindLastTileHit(float SpentTime, FIntPoint& OutTile) const;
};
