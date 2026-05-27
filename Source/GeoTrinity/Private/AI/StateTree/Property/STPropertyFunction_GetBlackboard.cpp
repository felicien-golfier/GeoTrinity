// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AI/StateTree/Property/STPropertyFunction_GetBlackboard.h"

#include "AI/GeoEnemyAIController.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeNodeDescriptionHelpers.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(STPropertyFunction_GetBlackboard)

#define LOCTEXT_NAMESPACE "StateTree"

void FSTGetBlackboardPropertyFunction::Execute(FStateTreeExecutionContext& Context) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	AGeoEnemyAIController const* Controller = Cast<AGeoEnemyAIController>(InstanceData.Input);
	if (!ensureMsgf(Controller && Controller->GeoBlackBoard,
					TEXT("FSTGetBlackboardPropertyFunction: controller or GeoBlackBoard is null")))
	{
		return;
	}

	InstanceData.Blackboard = Controller->GeoBlackBoard->Data;
}

#if WITH_EDITOR
FText FSTGetBlackboardPropertyFunction::GetDescription(FGuid const& ID, FStateTreeDataView InstanceDataView,
														IStateTreeBindingLookup const& BindingLookup,
														EStateTreeNodeFormatting Formatting) const
{
	return UE::StateTree::DescHelpers::GetDescriptionForSingleParameterFunc<FInstanceDataType>(
		LOCTEXT("GetBlackboard", "GetBlackboard"), ID, InstanceDataView, BindingLookup, Formatting);
}
#endif

#undef LOCTEXT_NAMESPACE
