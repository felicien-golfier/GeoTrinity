// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Encounter/GeoEncounterAction.h"

#include "GeoAddToCounterAction.generated.h"

/** Adds Delta to a step counter of its encounter. */
UCLASS()
class GEOTRINITY_API UGeoAddToCounterAction : public UGeoEncounterAction
{
	GENERATED_BODY()

public:
	virtual void Execute_Implementation(FGeoEncounterPayload const& Payload) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	FName CounterName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	int32 Delta = 1;
};
