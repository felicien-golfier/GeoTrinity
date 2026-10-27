// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Encounter/Action/GeoCatSpeechAction.h"
#include "Tool/CatLine/GeoCatLineCategories.h"

#include "GeoCatLineFromFileAction.generated.h"

/** The Cat says a line of Categories read from Data/CatLines.txt (Tool/CatLine/CLAUDE.md), a new one at every run. */
UCLASS()
class GEOTRINITY_API UGeoCatLineFromFileAction : public UGeoCatSpeechAction
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	/** The categories the line is drawn from; empty = all. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	FGeoCatLineCategories Categories;

	/** True: shuffled, no repeat until every line was said. False: the file's order, cycling. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	bool bRandom = true;

protected:
	virtual FText GetLineText() const override;
};
