// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Encounter/GeoEncounterAction.h"

#include "GeoLeaveTutorialAction.generated.h"

/** The tutorial's end: travels to the Training Room, as the pause menu's Leave tutorial does
 * (UGeoGameInstance::LeaveTutorial). */
UCLASS()
class GEOTRINITY_API UGeoLeaveTutorialAction : public UGeoEncounterAction
{
	GENERATED_BODY()

public:
	virtual void Execute_Implementation(FGeoEncounterPayload const& Payload) override;
};
