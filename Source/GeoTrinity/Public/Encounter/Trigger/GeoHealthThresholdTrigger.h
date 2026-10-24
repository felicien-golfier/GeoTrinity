// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Encounter/GeoEncounterTrigger.h"
#include "Encounter/Trigger/GeoActorFilter.h"

#include "GeoHealthThresholdTrigger.generated.h"

class AGeoCharacter;
struct FOnAttributeChangeData;

UENUM(BlueprintType)
enum class EGeoHealthSide : uint8
{
	Above,
	Below,
};

/**
 * Fires when the health ratio of a watched actor reaches Ratio from the chosen side (Above: >=, Below: <=), also from
 * OnArm when it is already there. Watched: every character matching the filter, the ones spawned while armed
 * included (watched one tick after their spawn, once their setup tag and starting health are set). Re-checked on every
 * Health or MaxHealth change. Each actor fires again only after its ratio left that side and came back; a MaxHealth of
 * 0 is on no side. Actor = the watched actor, Count = crossings since OnArm (all watched actors together), Value = the
 * ratio.
 */
UCLASS()
class GEOTRINITY_API UGeoHealthThresholdTrigger : public UGeoEncounterTrigger
{
	GENERATED_BODY()

public:
	virtual void OnArm_Implementation() override;
	virtual void OnDisarm_Implementation() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	FGeoActorFilter Watched;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	EGeoHealthSide Side = EGeoHealthSide::Below;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter", meta = (ClampMin = "0.0"))
	float Ratio = 0.5f;

private:
	/** Binds the Health and MaxHealth changes of Actor's ability system, then evaluates it. */
	void Watch(AActor& Actor);

	void OnActorSpawned(AActor* Actor);

	/** Next tick after a spawn: watches Actor when it matches and the trigger is still armed. */
	void WatchIfMatching(TWeakObjectPtr<AActor> Actor);

	void OnHealthOrMaxHealthChanged(FOnAttributeChangeData const& Data, TWeakObjectPtr<AActor> Actor);

	/** Fires when Actor's current health ratio is on the chosen side and was not before. */
	void Evaluate(AActor& Actor);

	/** Per watched actor: was its ratio on the chosen side at the last check. */
	TMap<TWeakObjectPtr<AActor>, bool> WasPast;

	/** Crossings fired since OnArm: the payload's Count. */
	int32 Crossings = 0;

	/** Valid while armed: OnArm sets it before its first evaluation, OnDisarm resets it. */
	FDelegateHandle ActorSpawnedHandle;
};
