// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"

#include "STTask_SelectNextFiringPoint.generated.h"

/** Per-instance data for FSTTask_SelectNextFiringPoint. TargetLocation is the output binding. */
USTRUCT()
struct GEOTRINITY_API FSTTask_SelectNextFiringPointInstanceData
{
	GENERATED_BODY()

	/** Output: world location of the selected firing point, bound to a StateTree output parameter. */
	UPROPERTY(EditAnywhere, Category = "Output")
	FVector TargetLocation = FVector::ZeroVector;
};

/**
 * StateTree task that selects the next firing point location from AEnemyCharacter (round-robin)
 */
USTRUCT(DisplayName = "Select Next Firing Point", Category = "GeoTrinity|AI")
struct GEOTRINITY_API FSTTask_SelectNextFiringPoint : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FSTTask_SelectNextFiringPointInstanceData;

	FSTTask_SelectNextFiringPoint() = default;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
