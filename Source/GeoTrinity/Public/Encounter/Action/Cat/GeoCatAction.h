// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Encounter/GeoEncounterAction.h"

#include "GeoCatAction.generated.h"

class AGeoCat;

/**
 * Base of the actions giving the placed Cat one order (walk, roam, montage). bWaitForEnd: finishes when the cat's order
 * ends (completed or replaced); else at once, so the next action (a Cat line) runs while the cat carries it out.
 */
UCLASS(Abstract)
class GEOTRINITY_API UGeoCatAction : public UGeoEncounterAction
{
	GENERATED_BODY()

public:
	UGeoCatAction() { bLatent = true; }

	virtual void Execute_Implementation(FGeoEncounterPayload const& Payload) override;

	/** Unbinds only: the cat keeps carrying out its order. */
	virtual void OnCancel_Implementation() override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	TObjectPtr<AGeoCat> Cat;

	/** True: finishes when the order ends. Ignored by an order that never ends on its own (roam). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	bool bWaitForEnd = true;

protected:
	/** Gives the order to OrderedCat. */
	virtual void GiveOrder(AGeoCat& OrderedCat) const PURE_VIRTUAL(UGeoCatAction::GiveOrder, );

	/** False for an order that only ends when replaced: the action never waits for it. */
	virtual bool EndsOnItsOwn() const { return true; }

private:
	void OnOrderEnded(bool bCompleted);

	void StopWaiting();

	FDelegateHandle OrderEndedHandle;
};
