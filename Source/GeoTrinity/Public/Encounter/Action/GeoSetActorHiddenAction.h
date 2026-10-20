// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Encounter/GeoEncounterAction.h"

#include "GeoSetActorHiddenAction.generated.h"

/** Hides or shows a level actor in game (SetActorHiddenInGame). */
UCLASS()
class GEOTRINITY_API UGeoSetActorHiddenAction : public UGeoEncounterAction
{
	GENERATED_BODY()

public:
	virtual void Execute_Implementation(FGeoEncounterPayload const& Payload) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	TObjectPtr<AActor> Actor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	bool bHidden = true;
};
