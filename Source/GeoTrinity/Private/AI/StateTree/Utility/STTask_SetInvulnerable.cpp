// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AI/StateTree/Utility/STTask_SetInvulnerable.h"

#include "Characters/GeoCharacter.h"
#include "StateTreeExecutionContext.h"
#include "Tool/UGeoGameplayLibrary.h"

FSTTask_SetInvulnerable::FSTTask_SetInvulnerable()
{
	bShouldCallTick = false;
}

EStateTreeRunStatus FSTTask_SetInvulnerable::EnterState(FStateTreeExecutionContext& Context,
														FStateTreeTransitionResult const& /*Transition*/) const
{
	AGeoCharacter* const Character = GetCharacter(Context);
	if (!ensureMsgf(Character, TEXT("STTask_SetInvulnerable: owner is not an AGeoCharacter.")))
	{
		return EStateTreeRunStatus::Failed;
	}

	Character->SetInvulnerable(Context.GetInstanceData(*this).bInvulnerable);
	return EStateTreeRunStatus::Running;
}

void FSTTask_SetInvulnerable::ExitState(FStateTreeExecutionContext& Context,
										FStateTreeTransitionResult const& /*Transition*/) const
{
	if (AGeoCharacter* const Character = GetCharacter(Context))
	{
		Character->SetInvulnerable(!Context.GetInstanceData(*this).bInvulnerable);
	}
}

AGeoCharacter* FSTTask_SetInvulnerable::GetCharacter(FStateTreeExecutionContext const& Context) const
{
	return Cast<AGeoCharacter>(GeoLib::ResolveOwnerPawn(Context.GetOwner()));
}
