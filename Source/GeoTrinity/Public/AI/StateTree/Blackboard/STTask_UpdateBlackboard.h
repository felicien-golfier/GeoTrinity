// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AI/GeoAIBlackboardComponent.h"
#include "CoreMinimal.h"
#include "StateTreeExecutionTypes.h"
#include "StateTreeTaskBase.h"

#include "STTask_UpdateBlackboard.generated.h"

class UGeoAIBlackboardComponent;

UENUM()
enum class EGeoBlackboardOp : uint8
{
	None,
	Set,
	Add,
	Multiply,
};

USTRUCT()
struct GEOTRINITY_API FGeoBlackboardIntFieldOp
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Parameter")
	EGeoBlackboardOp Op = EGeoBlackboardOp::None;

	UPROPERTY(EditAnywhere, Category = "Parameter",
			  meta = (EditCondition = "Op != EGeoBlackboardOp::None", EditConditionHides = false))
	int32 Value = 0;

	int32 Apply(int32 Current) const
	{
		switch (Op)
		{
		case EGeoBlackboardOp::Set:
			return Value;
		case EGeoBlackboardOp::Add:
			return Current + Value;
		case EGeoBlackboardOp::Multiply:
			return Current * Value;
		default:
			return Current;
		}
	}
};

USTRUCT()
struct GEOTRINITY_API FGeoBlackboardFloatFieldOp
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Parameter")
	EGeoBlackboardOp Op = EGeoBlackboardOp::None;

	UPROPERTY(EditAnywhere, Category = "Parameter",
			  meta = (EditCondition = "Op != EGeoBlackboardOp::None", EditConditionHides = false))
	float Value = 0.f;

	float Apply(float Current) const
	{
		switch (Op)
		{
		case EGeoBlackboardOp::Set:
			return Value;
		case EGeoBlackboardOp::Add:
			return Current + Value;
		case EGeoBlackboardOp::Multiply:
			return Current * Value;
		default:
			return Current;
		}
	}
};

USTRUCT()
struct GEOTRINITY_API FSTTask_UpdateBlackboardInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bReturnSucceeded = false;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	FGeoBlackboardIntFieldOp LastFiringPointIndex;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	FGeoBlackboardIntFieldOp CycleCount;
};

/**
 * Applies operations to selected AI blackboard fields. Set Op to anything other than None to activate a field.
 * Supports Set, Add, and Multiply. Value is a float to allow multiply ratios; truncated to int32 on apply.
 */
USTRUCT(DisplayName = "Update Blackboard", Category = "GeoTrinity|AI")
struct GEOTRINITY_API FSTTask_UpdateBlackboard : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()


	FSTTask_UpdateBlackboard() { bShouldCallTick = false; }

	using FInstanceDataType = FSTTask_UpdateBlackboardInstanceData;

	virtual UStruct const* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
										   FStateTreeTransitionResult const& Transition) const override;

private:
	TStateTreeExternalDataHandle<UGeoAIBlackboardComponent> BlackboardHandle;
};
