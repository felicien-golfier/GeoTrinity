// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Encounter/GeoEncounterTrigger.h"

#include "GeoTimerTrigger.generated.h"

/** Fires Delay seconds after arming (bLooping: every Delay). Count = firings so far. */
UCLASS()
class GEOTRINITY_API UGeoTimerTrigger : public UGeoEncounterTrigger
{
	GENERATED_BODY()

public:
	virtual void OnArm_Implementation() override;
	virtual void OnDisarm_Implementation() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter", meta = (ClampMin = "0.01"))
	float Delay = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	bool bLooping = false;

private:
	void OnTimer();

	FTimerHandle Timer;
	int32 Firings = 0;
};
