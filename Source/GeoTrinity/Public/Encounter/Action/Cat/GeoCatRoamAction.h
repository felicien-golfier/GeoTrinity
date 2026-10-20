// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Actor/GeoCat.h"
#include "CoreMinimal.h"
#include "Encounter/Action/Cat/GeoCatAction.h"

#include "GeoCatRoamAction.generated.h"

/** The Cat roams Settings.Area until another order replaces it; the action finishes at once. */
UCLASS()
class GEOTRINITY_API UGeoCatRoamAction : public UGeoCatAction
{
	GENERATED_BODY()

public:
	UGeoCatRoamAction() { bWaitForEnd = false; }

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	FGeoCatRoam Settings;

protected:
	virtual void GiveOrder(AGeoCat& OrderedCat) const override { OrderedCat.Roam(Settings); }
	virtual bool EndsOnItsOwn() const override { return false; }
};
