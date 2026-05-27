// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AI/GeoAIBlackboardComponent.h"
#include "AIController.h"
#include "CoreMinimal.h"
#include "StateTreePropertyFunctionBase.h"

#include "STPropertyFunction_GetBlackboard.generated.h"

USTRUCT()
struct FSTGetBlackboardPropertyFunctionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Parameter)
	TObjectPtr<AAIController> Input = nullptr;

	UPROPERTY(EditAnywhere, Category = Output)
	FGeoAIBlackboardData Blackboard;
};

/**
 * StateTree property function that reads CycleCount and LastFiringPointIndex from UGeoAIBlackboardComponent.
 * Bind the outputs to any condition or task input that needs BB values.
 */
USTRUCT(DisplayName = "Get Blackboard")
struct GEOTRINITY_API FSTGetBlackboardPropertyFunction : public FStateTreePropertyFunctionCommonBase
{
	GENERATED_BODY();

	using FInstanceDataType = FSTGetBlackboardPropertyFunctionInstanceData;

	virtual UStruct const* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual void Execute(FStateTreeExecutionContext& Context) const override;

#if WITH_EDITOR
	virtual FText GetDescription(FGuid const& ID, FStateTreeDataView InstanceDataView,
								 IStateTreeBindingLookup const& BindingLookup,
								 EStateTreeNodeFormatting Formatting) const override;
#endif
};
