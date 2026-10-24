// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Data/EffectData.h"
#include "Actor/Deployable/GeoDeployableBase.h"
#include "CoreMinimal.h"
#include "Encounter/GeoEncounterAction.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "Tool/Team.h"

#include "GeoSpawnZoneAction.generated.h"

class AGeoEffectZone;

/**
 * Spawns one effect zone (the project's DefaultZoneClass) on the first target point tagged PurposeTag and ArenaTag,
 * with the effects, size, colours, attitude and team written on the action. It has no owner: the zone is its own
 * effect source. Finishes at once and lasts until the step is left (held by the encounter), then expires. Run again
 * while held (a repeating rule): the previous zone is recalled first.
 */
UCLASS()
class GEOTRINITY_API UGeoSpawnZoneAction : public UGeoEncounterAction
{
	GENERATED_BODY()

public:
	virtual void Execute_Implementation(FGeoEncounterPayload const& Payload) override;

	/** The step was left, or the action runs again: recalls the zone, it blinks, then goes. */
	virtual void OnCancel_Implementation() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter", meta = (Categories = "TargetPoint"))
	FGameplayTag PurposeTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter", meta = (Categories = "Arena"))
	FGameplayTag ArenaTag;

	/** What the zone does to those inside. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	TArray<TInstancedStruct<FEffectData>> EffectDataArray;

	/** Size (radius), colours and attitude; LifeDrainMaxDuration stays 0 so the zone lasts until the step is left. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	FDeployableDataParams Params;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	ETeam Team = ETeam::Neutral;

private:
	TWeakObjectPtr<AGeoEffectZone> Zone;
};
