// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StateTreeExecutionTypes.h"
#include "StateTreeTaskBase.h"

#include "STTask_SendEventAfterNCycles.generated.h"

class UGeoAIBlackboardComponent;

USTRUCT()
struct GEOTRINITY_API FSTTask_SendEventAfterNCyclesInstanceData
{
	GENERATED_BODY()

	/** Number of cycles before the event is sent. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "1"))
	int32 CyclesRequired = 3;

	/** Event tag sent when the cycle count is reached. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FGameplayTag EventTag;

};

/**
 * Counts how many times the state has been entered. When CycleCount reaches CyclesRequired,
 * sends EventTag via the StateTree and resets the counter.
 * Always completes with Succeeded so the state it lives in proceeds normally.
 */
USTRUCT(DisplayName = "Send Event After N Cycles", Category = "GeoTrinity|AI")
struct GEOTRINITY_API FSTTask_SendEventAfterNCycles : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FSTTask_SendEventAfterNCyclesInstanceData;

	FSTTask_SendEventAfterNCycles();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
										   FStateTreeTransitionResult const& Transition) const override;

private:
	TStateTreeExternalDataHandle<UGeoAIBlackboardComponent> BlackboardHandle;
};
