// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Encounter/GeoEncounterObject.h"
#include "Encounter/GeoEncounterTypes.h"

#include "GeoEncounterAction.generated.h"

class IGeoHUDInterface;

/**
 * Base of every action: does one thing, in Execute, and the encounter runs the next one once it has finished. A
 * non-latent action finishes when Execute returns. A latent one (bLatent) must call Finish later and release its timers
 * and bindings in OnCancel, which the encounter calls when the step is left before it finished.
 */
UCLASS(Abstract)
class GEOTRINITY_API UGeoEncounterAction : public UGeoEncounterObject
{
	GENERATED_BODY()

public:
	/** Encounter only. Runs Execute, then Finish when the action is not latent. */
	void Begin(FGeoEncounterPayload const& Payload);

	UFUNCTION(BlueprintNativeEvent, Category = "GeoEncounter")
	void Execute(FGeoEncounterPayload const& Payload);
	virtual void Execute_Implementation(FGeoEncounterPayload const& /*Payload*/) {}

	/** Latent action: lets the encounter go on with the next action. Ignored when the encounter is not awaiting it. */
	UFUNCTION(BlueprintCallable, Category = "GeoEncounter")
	void Finish();

	UFUNCTION(BlueprintNativeEvent, Category = "GeoEncounter")
	void OnCancel();
	virtual void OnCancel_Implementation() {}

	/** Jump start: the step holding this action was skipped, not played (Encounter/CLAUDE.md, "Jump start").
	 * Overridden only by an action with a lasting effect on the player; never latent, no Finish. */
	UFUNCTION(BlueprintNativeEvent, Category = "GeoEncounter")
	void OnSkipped();
	virtual void OnSkipped_Implementation() {}

protected:
	/** The HUD of the first local player: in the solo tutorial (a listen host) the authority running the encounter
	 * and the player's UI share one process. Null, logged, when there is none. */
	IGeoHUDInterface* GetLocalHUD() const;

	/** True: Execute starts something that ends with a call to Finish. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoEncounter")
	bool bLatent = false;
};
