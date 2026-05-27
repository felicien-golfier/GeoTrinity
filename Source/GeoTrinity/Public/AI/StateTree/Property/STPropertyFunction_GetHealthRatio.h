// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "StateTreePropertyFunctionBase.h"

#include "STPropertyFunction_GetHealthRatio.generated.h"

USTRUCT()
struct FSTGetHealthRatioPropertyFunctionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Parameter)
	TObjectPtr<AAIController> Input = nullptr;

	UPROPERTY(EditAnywhere, Category = Output)
	float Output = 1.f;
};

/**
 * StateTree property function that reads the owning pawn's current health ratio from UGeoAttributeSetBase.
 * Inputs an AIController; outputs a float in [0, 1]. Outputs 0 if the controller or ASC is absent.
 */
USTRUCT(DisplayName = "Get Health Ratio")
struct GEOTRINITY_API FSTGetHealthRatioPropertyFunction : public FStateTreePropertyFunctionCommonBase
{
	GENERATED_BODY();

	using FInstanceDataType = FSTGetHealthRatioPropertyFunctionInstanceData;

	virtual UStruct const* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	/** Reads the controlled pawn's health ratio and writes it to InstanceData.Output. */
	virtual void Execute(FStateTreeExecutionContext& Context) const override;

#if WITH_EDITOR
	/** Returns a human-readable one-line summary of this function node for the StateTree editor. */
	virtual FText GetDescription(FGuid const& ID, FStateTreeDataView InstanceDataView,
								 IStateTreeBindingLookup const& BindingLookup,
								 EStateTreeNodeFormatting Formatting) const override;
#endif
};
