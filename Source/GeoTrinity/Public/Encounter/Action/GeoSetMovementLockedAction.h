// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Encounter/GeoEncounterAction.h"

#include "GeoSetMovementLockedAction.generated.h"

/**
 * Locks or unlocks the walk input of the first local player's controller. The encounters hold one lock
 * (UGeoEncounterSubsystem::bMovementLocked), never a count: a lock while locked, or an unlock while free, does nothing,
 * so a step re-entered by Restart cannot stack a second lock its single unlock leaves behind.
 */
UCLASS()
class GEOTRINITY_API UGeoSetMovementLockedAction : public UGeoEncounterAction
{
	GENERATED_BODY()

public:
	virtual void Execute_Implementation(FGeoEncounterPayload const& Payload) override;
	/** Same as Execute: a skipped lock then unlock leaves movement free. */
	virtual void OnSkipped_Implementation() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	bool bLocked = true;
};
