// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Encounter/GeoEncounterAction.h"

#include "GeoStartEncounterAction.generated.h"

class AGeoEncounter;

/** Starts another encounter at its first step through its arrival reset (AGeoEncounter::StartAfterSkippingChain); one
 * already running starts over. */
UCLASS()
class GEOTRINITY_API UGeoStartEncounterAction : public UGeoEncounterAction
{
	GENERATED_BODY()

public:
	virtual void Execute_Implementation(FGeoEncounterPayload const& Payload) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	TObjectPtr<AGeoEncounter> Target;
};
