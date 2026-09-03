// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AI/StateTree/Utility/STTask_PlayMontage.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Animation/AnimInstance.h"
#include "StateTreeAsyncExecutionContext.h"
#include "StateTreeExecutionContext.h"
#include "Tool/UGeoGameplayLibrary.h"

FSTTask_PlayMontage::FSTTask_PlayMontage()
{
	bShouldCallTick = false;
}

EStateTreeRunStatus FSTTask_PlayMontage::EnterState(FStateTreeExecutionContext& Context,
													FStateTreeTransitionResult const& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	UGeoAbilitySystemComponent* const ASC = GetASC(Context);
	if (!ensureMsgf(InstanceData.Montage, TEXT("STTask_PlayMontage: Montage is not set.")) ||
		!ensureMsgf(ASC, TEXT("STTask_PlayMontage: pawn has no AbilitySystemComponent.")))
	{
		return EStateTreeRunStatus::Failed;
	}

	if (ASC->PlayMontage(nullptr, FGameplayAbilityActivationInfo(), InstanceData.Montage, InstanceData.PlayRate) <= 0.f)
	{
		return EStateTreeRunStatus::Failed;
	}

	FOnMontageEnded EndDelegate;
	EndDelegate.BindLambda(
		[WeakContext = Context.MakeWeakExecutionContext()](UAnimMontage* /*EndedMontage*/, bool const bInterrupted)
		{
			WeakContext.FinishTask(bInterrupted ? EStateTreeFinishTaskType::Failed
												: EStateTreeFinishTaskType::Succeeded);
		});
	ASC->AbilityActorInfo->GetAnimInstance()->Montage_SetEndDelegate(EndDelegate, InstanceData.Montage);

	return EStateTreeRunStatus::Running;
}

void FSTTask_PlayMontage::ExitState(FStateTreeExecutionContext& Context,
									FStateTreeTransitionResult const& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	UGeoAbilitySystemComponent* const ASC = GetASC(Context);
	if (ASC && InstanceData.Montage)
	{
		ASC->StopMontageIfCurrent(*InstanceData.Montage);
	}
}

UGeoAbilitySystemComponent* FSTTask_PlayMontage::GetASC(FStateTreeExecutionContext const& Context) const
{
	return GeoASLib::GetGeoAscFromActor(GeoLib::ResolveOwnerPawn(Context.GetOwner()));
}
