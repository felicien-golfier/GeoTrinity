// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Encounter/GeoEncounterAction.h"
#include "ScalableFloat.h"

#include "GeoDelayAction.generated.h"

/** Waits Duration seconds (a scalable float: a CT_Tutorial row keeps a timed script in step with its lines). */
UCLASS()
class GEOTRINITY_API UGeoDelayAction : public UGeoEncounterAction
{
	GENERATED_BODY()

public:
	UGeoDelayAction() { bLatent = true; }

	virtual void Execute_Implementation(FGeoEncounterPayload const& Payload) override;
	virtual void OnCancel_Implementation() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter", meta = (ClampMin = "0.0"))
	FScalableFloat Duration = FScalableFloat(1.f);

private:
	FTimerHandle Timer;
};
