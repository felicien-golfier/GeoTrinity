// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Encounter/GeoEncounterTrigger.h"
#include "Encounter/Trigger/GeoActorFilter.h"

#include "GeoActorDiedTrigger.generated.h"

class AGeoCharacter;

/**
 * Fires when a character matching Watched dies (APlayableCharacter = player deaths). Count = deaths so far, Value =
 * watched characters still alive.
 */
UCLASS()
class GEOTRINITY_API UGeoActorDiedTrigger : public UGeoEncounterTrigger
{
	GENERATED_BODY()

public:
	virtual void OnArm_Implementation() override;
	virtual void OnDisarm_Implementation() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	FGeoActorFilter Watched;

private:
	void OnCharacterDied(AGeoCharacter& Character);

	int32 Deaths = 0;
};
