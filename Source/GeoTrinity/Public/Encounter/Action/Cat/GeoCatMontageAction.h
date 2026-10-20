// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Encounter/Action/Cat/GeoCatAction.h"

#include "GeoCatMontageAction.generated.h"

class UAnimMontage;

/**
 * The Cat plays Montage. Its order ends when the montage starts blending out: a hide right after it in the same list
 * lands before the blend back to locomotion shows.
 */
UCLASS()
class GEOTRINITY_API UGeoCatMontageAction : public UGeoCatAction
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	TObjectPtr<UAnimMontage> Montage;

protected:
	virtual void GiveOrder(AGeoCat& OrderedCat) const override;
};
