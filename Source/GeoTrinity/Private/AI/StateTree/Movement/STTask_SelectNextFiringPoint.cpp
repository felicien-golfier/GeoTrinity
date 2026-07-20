// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AI/StateTree/Movement/STTask_SelectNextFiringPoint.h"

#include "AI/GeoAIBlackboardComponent.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "Actor/GeoArena.h"
#include "GameFramework/Controller.h"
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

	AActor const* Owner = Cast<AActor>(Context.GetOwner());
	AController const* Controller = Cast<AController>(Owner);
	AGeoArena const* Arena = AGeoArena::GetArenaOfBoss(Controller ? Controller->GetPawn() : Owner);
	if (!ensureMsgf(Arena, TEXT("FSTTask_SelectNextFiringPoint: %s was not spawned by an arena"), *GetNameSafe(Owner)))
	{
		return EStateTreeRunStatus::Failed;
	}

	TArray<AActor*> FiringPoints =
		GeoLib::GetTargetPoints(Owner, FGeoGameplayTags::Get().TargetPoint_FiringPoint, Arena->ArenaTag);

	FiringPoints.Sort(
		[](AActor const& A, AActor const& B)
		{
			return A.GetUniqueID() < B.GetUniqueID();
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
