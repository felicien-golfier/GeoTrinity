// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Encounter/GeoEncounterObject.h"
#include "Encounter/GeoEncounterTypes.h"

#include "GeoEncounterCondition.generated.h"

/** Filter on a rule: decides which firings of its trigger count. */
UCLASS(Abstract)
class GEOTRINITY_API UGeoEncounterCondition : public UGeoEncounterObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "GeoEncounter")
	bool Evaluate(FGeoEncounterPayload const& Payload) const;
	virtual bool Evaluate_Implementation(FGeoEncounterPayload const& /*Payload*/) const { return true; }
};
