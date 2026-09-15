// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"

#include "GeoAIBlackboardComponent.generated.h"

/**
 * All persistent AI state for the enemy StateTree.
 * Exposed as a struct so FSTTask_UpdateBlackboard can write any subset of fields from the editor.
 */
USTRUCT(BlueprintType)
struct GEOTRINITY_API FGeoAIBlackboardData
{
	GENERATED_BODY()

	/** Last selected firing point index — written by FSTTask_SelectNextFiringPoint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AActor* LastFiringPointActor = nullptr;

	/** Cycle counter — written by FSTTask_SendEventAfterNCycles. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CycleCount = 0;

	/** True from this enemy's aggro, before its intro plays — written by AGeoEnemyAIController. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAggroed = false;

	/** True once this enemy's fight has started (after its intro, if any) — written by AGeoEnemyAIController. A state,
	 * not an event, so a tree back at its root still reads it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bFightStarted = false;
};

/**
 * Persistent AI state for the enemy StateTree. Attached to AGeoEnemyAIController.
 * The StateTree schema resolves UActorComponent subclasses automatically, so any task
 * can link to this via TStateTreeExternalDataHandle<UGeoAIBlackboardComponent>.
 */
UCLASS()
class GEOTRINITY_API UGeoAIBlackboardComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FGeoAIBlackboardData Data;
};
