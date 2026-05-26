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

USTRUCT(DisplayName = "Get Health Ratio")
struct GEOTRINITY_API FSTGetHealthRatioPropertyFunction : public FStateTreePropertyFunctionCommonBase
{
	GENERATED_BODY();

	using FInstanceDataType = FSTGetHealthRatioPropertyFunctionInstanceData;

	virtual UStruct const* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual void Execute(FStateTreeExecutionContext& Context) const override;

#if WITH_EDITOR
	virtual FText GetDescription(FGuid const& ID, FStateTreeDataView InstanceDataView,
								 IStateTreeBindingLookup const& BindingLookup,
								 EStateTreeNodeFormatting Formatting) const override;
#endif
};
