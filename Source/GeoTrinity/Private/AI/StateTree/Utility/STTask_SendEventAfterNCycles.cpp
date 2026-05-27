// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AI/StateTree/Utility/STTask_SendEventAfterNCycles.h"

#include "AI/GeoAIBlackboardComponent.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"

FSTTask_SendEventAfterNCycles::FSTTask_SendEventAfterNCycles()
{
	bShouldCallTick = false;
}

bool FSTTask_SendEventAfterNCycles::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(BlackboardHandle);
	return true;
}

EStateTreeRunStatus FSTTask_SendEventAfterNCycles::EnterState(FStateTreeExecutionContext& Context,
	FStateTreeTransitionResult const& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (!ensureMsgf(InstanceData.EventTag.IsValid(), TEXT("STTask_SendEventAfterNCycles: EventTag is not set.")))
	{
		return EStateTreeRunStatus::Failed;
	}

	UGeoAIBlackboardComponent& Blackboard = Context.GetExternalData(BlackboardHandle);
	++Blackboard.Data.CycleCount;

	if (Blackboard.Data.CycleCount >= InstanceData.CyclesRequired)
	{
		Blackboard.Data.CycleCount = 0;
		Context.SendEvent(InstanceData.EventTag);
	}

	return EStateTreeRunStatus::Succeeded;
}
