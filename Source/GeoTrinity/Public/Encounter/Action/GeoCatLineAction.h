// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Encounter/GeoEncounterAction.h"

#include "GeoCatLineAction.generated.h"

class AGeoCharacter;

/**
 * Says Line, the Cat's text in the Tutorial Board markup (**bold**, ~~shake~~, {Key:<InputTag>}; {Counter:<Name>} = the
 * value of that step counter of its encounter), on the local player's HUD. With bWaitForRespawn, a line fired while
 * that player is dead shows once it is revived. Latent: finishes when the line ends.
 */
UCLASS()
class GEOTRINITY_API UGeoCatLineAction : public UGeoEncounterAction
{
	GENERATED_BODY()

public:
	/** Sets bLatent true — this action waits until the line ends before the encounter advances. */
	UGeoCatLineAction() { bLatent = true; }

	/** Shows the line on the local player's HUD; waits for respawn first when bWaitForRespawn is set and the player is dead. */
	virtual void Execute_Implementation(FGeoEncounterPayload const& Payload) override;

	/** Stops waiting for the respawn or the line's end. A line on screen stays until it ends, unless the encounter is
	 * aborting: then it is ended at once. */
	virtual void OnCancel_Implementation() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter", meta = (MultiLine = true))
	FText Line;

	/** True: a line reacting to a death waits for the respawn instead of showing over the death screen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	bool bWaitForRespawn = false;

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
