// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Encounter/GeoEncounterTrigger.h"

#include "GeoPlayerIdleTrigger.generated.h"

/**
 * Fires every Delay seconds the local player stays within Radius of where it stood at the last firing, at arming or at
 * its last move. Count = idle tier: 1 at the first firing, one more at each next; a move or a death starts over at 0.
 */
UCLASS()
class GEOTRINITY_API UGeoPlayerIdleTrigger : public UGeoEncounterTrigger
{
	GENERATED_BODY()

public:
	virtual void OnArm_Implementation() override;
	virtual void OnDisarm_Implementation() override;

	/** Seconds of standing still between two firings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter", meta = (ClampMin = "0.5"))
	float Delay = 8.f;

	/** Moving less than this from where the player stood still counts as standing still. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter", meta = (ClampMin = "1"))
	float Radius = 100.f;

	/** Seconds between two looks at the player. */
	static constexpr float PollSeconds = .25f;

private:
	void Poll();

	FTimerHandle PollTimer;
	FVector StillLocation = FVector::ZeroVector;
	float StillSeconds = 0.f;
	int32 Tier = 0;
};
