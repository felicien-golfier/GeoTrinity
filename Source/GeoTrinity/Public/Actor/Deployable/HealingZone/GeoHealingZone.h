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
 * Everything else — capsule, tracking, replicated data, the authored EffectDataArray — is AGeoEffectZone's, including
 * who counts as an ally: only actors matching Params.Attitude are ever tracked, so nothing filters teams here.
 */
UCLASS(Blueprintable, ClassGroup = (Custom))
class GEOTRINITY_API AGeoHealingZone : public AGeoEffectZone
{
	GENERATED_BODY()

public:
	/** Forwards the object initializer: AGeoEffectZone has no default constructor for UHT to generate one from. */
	AGeoHealingZone(FObjectInitializer const& ObjectInitializer) : Super(ObjectInitializer) {}

protected:
	/** Heals a hurt ally at the zone's drain rate and charges the zone the same amount, on top of Super's effects. */
	virtual void ApplyZoneEffects(TWeakObjectPtr<AActor> const& TrackedActor,
								  UGeoAbilitySystemComponent* SourceASC) override;
};
