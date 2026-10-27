// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Encounter/Action/GeoCatSpeechAction.h"

#include "GeoCatLineAction.generated.h"

/** The Cat says Line, the text typed on the action itself. */
UCLASS()
class GEOTRINITY_API UGeoCatLineAction : public UGeoCatSpeechAction
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter", meta = (MultiLine = true))
	FText Line;

protected:
	virtual FText GetLineText() const override { return Line; }
};
