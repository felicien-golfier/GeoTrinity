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

	/** Hides the actor again when this action showed it (actors are placed hidden); a hide has nothing to undo. */
	virtual void OnCancel_Implementation() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	TObjectPtr<AActor> Actor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	bool bHidden = true;

private:
	/** The last Execute showed the actor and no cancel has hidden it since. */
	bool bShowedActor = false;
};
