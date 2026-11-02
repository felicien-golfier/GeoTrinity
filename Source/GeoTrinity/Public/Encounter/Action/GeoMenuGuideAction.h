// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Encounter/GeoEncounterAction.h"
#include "HUD/Interface/GeoMenuGuideGoal.h"

#include "GeoMenuGuideAction.generated.h"

/**
 * Starts the menu guide towards Goal on the local player's HUD: everything of the pause menu is dimmed and refuses
 * clicks but the next control on the way. Finishes at once and the guide lasts until the step is left (held by the
 * encounter), then stops. The player opens the menu; the action never does.
 */
UCLASS()
class GEOTRINITY_API UGeoMenuGuideAction : public UGeoEncounterAction
{
	GENERATED_BODY()

public:
	/** Starts the guide; held by the encounter until the step is left. */
	virtual void Execute_Implementation(FGeoEncounterPayload const& Payload) override;

	/** The step was left, or the action runs again: stops the guide. */
	virtual void OnCancel_Implementation() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	EGeoMenuGuideGoal Goal = EGeoMenuGuideGoal::None;
};
