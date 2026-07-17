// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AI/StateTree/Movement/STTask_MoveTo.h"

#include "AIController.h"
#include "AI/StateTree/Movement/GeoAITask_MoveTo.h"
#include "StateTreeExecutionContext.h"
#include "Tasks/AITask.h"

// ---------------------------------------------------------------------------------------------------------------------
UAITask_MoveTo* FSTTask_MoveTo::PrepareMoveToTask(FStateTreeExecutionContext& Context, AAIController& Controller,
                                                   UAITask_MoveTo* ExistingTask, FAIMoveRequest& MoveRequest) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	UGeoAITask_MoveTo* MoveTask = ExistingTask ? Cast<UGeoAITask_MoveTo>(ExistingTask)
	                                            : UAITask::NewAITask<UGeoAITask_MoveTo>(Controller, *InstanceData.TaskOwner, EAITaskPriority::High);
	if (!ensureMsgf(MoveTask, TEXT("FSTTask_MoveTo: failed to create UGeoAITask_MoveTo")))
	{
		return nullptr;
	}

	MoveTask->MoveGameplayCueTag = InstanceData.MoveGameplayCueTag;
	MoveTask->SetUp(&Controller, MoveRequest);
	return MoveTask;
}
