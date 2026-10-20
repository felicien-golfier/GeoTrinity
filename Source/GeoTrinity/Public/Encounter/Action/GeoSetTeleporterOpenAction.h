// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Encounter/GeoEncounterAction.h"

#include "GeoSetTeleporterOpenAction.generated.h"

class AGeoTeleporter;

/** Opens or closes a teleporter pad. */
UCLASS()
class GEOTRINITY_API UGeoSetTeleporterOpenAction : public UGeoEncounterAction
{
	GENERATED_BODY()

public:
	virtual void Execute_Implementation(FGeoEncounterPayload const& Payload) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	TObjectPtr<AGeoTeleporter> Teleporter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	bool bOpen = true;
};
