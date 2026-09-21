// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Actor/Deployable/Zones/GeoEffectZone.h"
#include "CoreMinimal.h"

#include "GeoHealingZone.generated.h"


/**
 * Deployable healing zone placed by the Circle player.
 * A zone whose heal is not authored but taken from its own life: every ally inside is healed at the zone's drain rate
 * and the zone pays that same amount out of its health, so how long it lasts is how much it healed. Can be absorbed by
 * UGeoMoiraBeamAbility which drains its health and converts it into fuel, radius growth, and damage/heal boost.
 *
 * The authored EffectDataArray is still judged by AGeoEffectZone; the heal is not, since helping an ally is no hazard to
 * lag-compensate — it goes to whoever stands inside on the server now.
 */
UCLASS(Blueprintable, ClassGroup = (Custom))
class GEOTRINITY_API AGeoHealingZone : public AGeoEffectZone
{
	GENERATED_BODY()

public:
	/** Forwards the object initializer: AGeoEffectZone has no default constructor for UHT to generate one from. */
	AGeoHealingZone(FObjectInitializer const& ObjectInitializer) : Super(ObjectInitializer) {}

protected:
	/** Server, until the zone blinks: heals every ally Params.Attitude matches inside the zone. */
	virtual void Tick(float DeltaSeconds) override;

private:
	/** Heals Ally at the zone's drain rate for this tick and charges the zone the same amount, unless Ally is already
	 * at full life. */
	void HealAlly(AActor* Ally, UGeoAbilitySystemComponent* SourceASC);
};
