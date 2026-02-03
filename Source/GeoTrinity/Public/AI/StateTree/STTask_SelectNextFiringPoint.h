// STTask_SelectNextFiringPoint.h
#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"

#include "STTask_SelectNextFiringPoint.generated.h"

USTRUCT()
struct GEOTRINITY_API FSTTask_SelectNextFiringPointInstanceData
{
	GENERATED_BODY()

	// Output: The selected firing point location
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
