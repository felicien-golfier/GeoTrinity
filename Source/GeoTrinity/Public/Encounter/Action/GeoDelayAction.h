// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Encounter/GeoEncounterAction.h"

#include "GeoDelayAction.generated.h"

/** Waits Duration seconds. */
UCLASS()
class GEOTRINITY_API UGeoDelayAction : public UGeoEncounterAction
{
	GENERATED_BODY()

public:
	UGeoDelayAction() { bLatent = true; }

	virtual void Execute_Implementation(FGeoEncounterPayload const& Payload) override;
	virtual void OnCancel_Implementation() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter", meta = (ClampMin = "0.0"))
	float Duration = 1.f;

private:
	FTimerHandle Timer;
};
