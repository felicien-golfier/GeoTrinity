// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AI/StateTree/Movement/STTask_SelectNextFiringPoint.h"

#include "AI/GeoAIBlackboardComponent.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "Engine/TargetPoint.h"
#include "Kismet/GameplayStatics.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"
#include "Tool/UGeoGameplayLibrary.h"

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

	TArray<AActor*> FiringPoints = GeoLib::GetTargetPoints(Context.GetOwner(), FGeoGameplayTags::Get().AI_FiringPoint);

	FiringPoints.Sort(
		[](TObjectPtr<AActor> const A, TObjectPtr<AActor> const B)
		{
			return A->GetUniqueID() < B->GetUniqueID();
		});

	int32 Num = FiringPoints.Num();
	if (Num <= 0)
	{
		return EStateTreeRunStatus::Failed;
	}

	if (Num > 1 && IsValid(Blackboard.Data.LastFiringPointActor))
	{
		FiringPoints.Remove(Blackboard.Data.LastFiringPointActor);
		Num = FiringPoints.Num();
	}

	int32 const Index = FMath::RandRange(0, Num - 1);
	if (IsValid(FiringPoints[Index]))
	{
		InstanceData.TargetLocation = FiringPoints[Index]->GetActorLocation();
		Blackboard.Data.LastFiringPointActor = FiringPoints[Index];
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Failed;
}
