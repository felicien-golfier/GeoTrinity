// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Encounter/GeoEncounterTrigger.h"
#include "GameplayTagContainer.h"

#include "GeoEventTrigger.generated.h"

/** Fires when UGeoEncounterSubsystem raises Tag (or a child of it), with the raised payload untouched. */
UCLASS()
class GEOTRINITY_API UGeoEventTrigger : public UGeoEncounterTrigger
{
	GENERATED_BODY()

public:
	virtual void OnArm_Implementation() override;
	virtual void OnDisarm_Implementation() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter", meta = (Categories = "Encounter.Event"))
	FGameplayTag Tag;

private:
	void OnRaised(FGeoEncounterPayload const& Payload);
};
