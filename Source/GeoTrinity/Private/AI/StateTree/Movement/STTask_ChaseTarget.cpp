// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AI/StateTree/Movement/STTask_ChaseTarget.h"

#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AIController.h"
#include "Characters/PlayableCharacter.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FSTTask_ChaseTarget::Tick(FStateTreeExecutionContext& Context, float const /*DeltaTime*/) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	APawn* Pawn = InstanceData.AIController ? InstanceData.AIController->GetPawn() : nullptr;
	if (!ensureMsgf(Pawn, TEXT("ChaseTarget: AIController has no pawn")))
	{
		return EStateTreeRunStatus::Failed;
	}

	FVector2D const PawnLocation(Pawn->GetActorLocation());
	APlayableCharacter* Target = nullptr;
	bool bTargetPreferred = false;
	float BestDistanceSquared = 0.f;
	for (APlayableCharacter* Player : GeoASLib::GetInteractableActors<APlayableCharacter>(
			 Pawn, GeoASLib::GetTeamId(Pawn), TeamAttitudeMask::Hostile, /*bMustBeDamageable*/ false, PawnLocation,
			 /*MaxDistance*/ 0.f))
	{
		if (Player->IsDead())
		{
			continue;
		}
		bool const bPreferred = Player->GetPlayerClass() == InstanceData.PreferredTargetClass;
		float const DistanceSquared = FVector2D::DistSquared(FVector2D(Player->GetActorLocation()), PawnLocation);
		if (!Target || (bPreferred && !bTargetPreferred)
			|| (bPreferred == bTargetPreferred && DistanceSquared < BestDistanceSquared))
		{
			Target = Player;
			bTargetPreferred = bPreferred;
			BestDistanceSquared = DistanceSquared;
		}
	}
	if (!Target)
	{
		return EStateTreeRunStatus::Running;
	}

	FVector2D const ToTarget = FVector2D(Target->GetActorLocation()) - PawnLocation;
	// SetControlRotation is overwritten every frame by AAIController::UpdateControlRotation; focus is the input it reads.
	InstanceData.AIController->SetFocus(Target);
	if (ToTarget.SizeSquared() > FMath::Square(InstanceData.StopDistance))
	{
		Pawn->AddMovementInput(FVector(ToTarget.GetSafeNormal(), 0.f));
	}
	return EStateTreeRunStatus::Running;
}
