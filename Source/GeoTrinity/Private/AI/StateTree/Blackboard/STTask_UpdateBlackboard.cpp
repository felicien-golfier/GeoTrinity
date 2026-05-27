// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AI/StateTree/Blackboard/STTask_UpdateBlackboard.h"

#include "AI/GeoAIBlackboardComponent.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"

bool FSTTask_UpdateBlackboard::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(BlackboardHandle);
	return true;
}

EStateTreeRunStatus FSTTask_UpdateBlackboard::EnterState(FStateTreeExecutionContext& Context,
														 FStateTreeTransitionResult const& Transition) const
{
	FInstanceDataType const& InstanceData = Context.GetInstanceData(*this);
	FGeoAIBlackboardData& Data = Context.GetExternalData(BlackboardHandle).Data;

	Data.LastFiringPointIndex = InstanceData.LastFiringPointIndex.Apply(Data.LastFiringPointIndex);
	Data.CycleCount = InstanceData.CycleCount.Apply(Data.CycleCount);

	return InstanceData.bReturnSucceeded ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}
