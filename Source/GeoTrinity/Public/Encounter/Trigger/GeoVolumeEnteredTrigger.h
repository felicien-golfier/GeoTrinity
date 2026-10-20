// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Encounter/GeoEncounterTrigger.h"

#include "GeoVolumeEnteredTrigger.generated.h"

/** Fires when a player overlaps Volume, also from OnArm for each player already inside. Count = entries so far. */
UCLASS()
class GEOTRINITY_API UGeoVolumeEnteredTrigger : public UGeoEncounterTrigger
{
	GENERATED_BODY()

public:
	virtual void OnArm_Implementation() override;
	virtual void OnDisarm_Implementation() override;

	/** Any level actor with a collision shape: a trigger box, an arena volume. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	TObjectPtr<AActor> Volume;

private:
	UFUNCTION()
	void OnVolumeBeginOverlap(AActor* OverlappedActor, AActor* OtherActor);

	void FireForPlayer(AActor& Player);

	int32 Entries = 0;
};
