// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Encounter/GeoEncounterObject.h"
#include "Encounter/GeoEncounterTypes.h"

#include "GeoEncounterTrigger.generated.h"

/**
 * Base of every trigger. Binds what it waits for in OnArm, unbinds in OnDisarm, and reports each occurrence with
 * Fire. The encounter arms it on step entry and disarms it on step exit, on Restart and once a non-repeating rule has
 * fired. A trigger on a state fires from OnArm when the state already holds; one on an event only on the next
 * occurrence. Reset any state in OnArm, never in a constructor.
 */
UCLASS(Abstract)
class GEOTRINITY_API UGeoEncounterTrigger : public UGeoEncounterObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "GeoEncounter")
	void OnArm();
	virtual void OnArm_Implementation() {}

	UFUNCTION(BlueprintNativeEvent, Category = "GeoEncounter")
	void OnDisarm();
	virtual void OnDisarm_Implementation() {}

	/** Reports an occurrence to the encounter, which tests the rule's condition and runs its actions. */
	UFUNCTION(BlueprintCallable, Category = "GeoEncounter")
	void Fire(FGeoEncounterPayload const& Payload);
};
