// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Encounter/GeoEncounterAction.h"
#include "ScalableFloat.h"

#include "GeoCatSpeechAction.generated.h"

class AGeoCharacter;

/**
 * Base of the actions making the Cat say a line: shows the text of GetLineText, in the Tutorial Board markup (**bold**,
 * ~~shake~~, {Key:<InputTag>}; {Counter:<Name>} = the value of that step counter of its encounter), on the local
 * player's HUD. With bWaitForRespawn, a line fired while that player is dead shows once it is revived. Latent: finishes
 * when the line ends, at once when GetLineText is empty. With bTimed it is not latent: the line lives Seconds, cannot
 * be skipped, and the action finishes as it starts.
 */
UCLASS(Abstract)
class GEOTRINITY_API UGeoCatSpeechAction : public UGeoEncounterAction
{
	GENERATED_BODY()

public:
	/** Sets bLatent true — this action waits until the line ends before the encounter advances. */
	UGeoCatSpeechAction() { bLatent = true; }

#if WITH_EDITOR
	/** A timed line whose text cannot type within Seconds is an error. */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	/** Shows the line on the local player's HUD; waits for respawn first when bWaitForRespawn is set and the player is
	 * dead. */
	virtual void Execute_Implementation(FGeoEncounterPayload const& Payload) override;

	/** Stops waiting for the respawn or the line's end. A line on screen stays until it ends, unless the encounter is
	 * aborting: then it is ended at once. */
	virtual void OnCancel_Implementation() override;

	/** True: a line reacting to a death waits for the respawn instead of showing over the death screen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	bool bWaitForRespawn = false;

	/** True: the line shows for exactly Seconds, unskippable, and the action finishes at once (a scripted timeline). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	bool bTimed = false;

	/** The life of a timed line, in seconds (a CT_Tutorial row). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter", meta = (EditCondition = "bTimed"))
	FScalableFloat Seconds;

protected:
	/** The text to say, asked once each time a line is about to show. Empty: nothing is shown and the action finishes.
	 */
	virtual FText GetLineText() const PURE_VIRTUAL(UGeoCatSpeechAction::GetLineText, return FText::GetEmpty(););

	/** The longest text this action can say, for the timed-line data check. */
	virtual FText GetLongestLineText() const { return GetLineText(); }

private:
	void ShowLine();

	/** The dead player whose revive shows the line; unset while nothing waits. */
	TWeakObjectPtr<AGeoCharacter> WaitedPlayer;

	/** Bumped on cancel: the end of a line shown by a cancelled run no longer finishes the action. */
	int32 LineRun = 0;

	/** Lines this action has shown; the end of an older one no longer clears bLineShowing. */
	int32 ShownLines = 0;

	/** The line this action showed is still on the HUD (it outlives a cancelled run until it ends). */
	bool bLineShowing = false;
};
