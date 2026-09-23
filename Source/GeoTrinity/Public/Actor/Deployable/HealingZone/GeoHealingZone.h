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
 * The heal rides AGeoEffectZone's judge like its authored EffectDataArray: each ally is healed for the time the judge
 * found it inside.
 */
UCLASS(Blueprintable, ClassGroup = (Custom))
class GEOTRINITY_API AGeoHealingZone : public AGeoEffectZone
{
	GENERATED_BODY()

public:
	/** Forwards the object initializer: AGeoEffectZone has no default constructor for UHT to generate one from. */
	AGeoHealingZone(FObjectInitializer const& ObjectInitializer) : Super(ObjectInitializer) {}

protected:
	/** Heals every ally the judge found inside the zone. */
	virtual void OnZoneJudged(TArray<FGeoHazardJudge::FTargetResult> const& Results) override;

private:
	/** Heals Ally at the zone's drain rate for TimeInside and charges the zone the same amount, unless Ally is already
	 * at full life. */
	void HealAlly(AActor* Ally, UGeoAbilitySystemComponent* SourceASC, float TimeInside);
};
