// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AI/StateTree/Movement/STTask_SelectNextFiringPoint.h"

#include "AI/GeoAIBlackboardComponent.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "Engine/TargetPoint.h"
#include "Kismet/GameplayStatics.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"

bool FSTTask_SelectNextFiringPoint::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(BlackboardHandle);
	return true;
}

EStateTreeRunStatus FSTTask_SelectNextFiringPoint::EnterState(FStateTreeExecutionContext& Context,
	FStateTreeTransitionResult const& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	UGeoAIBlackboardComponent& Blackboard = Context.GetExternalData(BlackboardHandle);

	TArray<AActor*> FiringPoints;
	UGameplayStatics::GetAllActorsOfClassWithTag(Context.GetWorld(), ATargetPoint::StaticClass(),
		FGeoGameplayTags::Get().AI_FiringPoint.GetTagName(), FiringPoints);
	int32 Num = FiringPoints.Num();
	if (Num <= 0)
	{
		return EStateTreeRunStatus::Failed;
	}

	if (Num > 1)
	{
		FiringPoints.RemoveAt(Blackboard.Data.LastFiringPointIndex);
		Num = FiringPoints.Num();
	}

	int32 const Index = FMath::RandRange(0, Num - 1);
	if (IsValid(FiringPoints[Index]))
	{
		InstanceData.TargetLocation = FiringPoints[Index]->GetActorLocation();
		Blackboard.Data.LastFiringPointIndex = Index;
		return EStateTreeRunStatus::Unset;
	}

	return EStateTreeRunStatus::Failed;
}
