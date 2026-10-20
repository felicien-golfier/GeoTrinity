// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Encounter/GeoEncounterTrigger.h"

#include "GeoCounterTrigger.generated.h"

/** Fires when the named counter of its encounter changes. Count = the new value. */
UCLASS()
class GEOTRINITY_API UGeoCounterTrigger : public UGeoEncounterTrigger
{
	GENERATED_BODY()

public:
	virtual void OnArm_Implementation() override;
	virtual void OnDisarm_Implementation() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	FName CounterName;

private:
	void OnCounterChanged(FName Counter, int32 NewValue);
};
