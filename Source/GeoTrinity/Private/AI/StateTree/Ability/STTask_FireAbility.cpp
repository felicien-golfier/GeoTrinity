// STTask_FireProjectileAbility.cpp

#include "AI/StateTree/Ability/STTask_FireAbility.h"

#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "StateTreeAsyncExecutionContext.h"
#include "StateTreeExecutionContext.h"

FSTTask_FireAbility::FSTTask_FireAbility()
{
	bShouldCallTick = false;
}

EStateTreeRunStatus FSTTask_FireAbility::EnterState(FStateTreeExecutionContext& Context,
													FStateTreeTransitionResult const& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	UAbilitySystemComponent* ASC = GetASC(Context);
	if (!ASC || !InstanceData.AbilityTag.IsValid())
	{
		return EStateTreeRunStatus::Failed;
	}

	// Bind to ability ended delegate before activating
	InstanceData.AbilityEndedDelegateHandle = ASC->OnAbilityEnded.AddLambda(
		[WeakContext = Context.MakeWeakExecutionContext(),
		 Tag = InstanceData.AbilityTag](FAbilityEndedData const& EndedData)
		{
			if (EndedData.AbilityThatEnded && EndedData.AbilityThatEnded->GetAssetTags().HasTag(Tag))
			{
				WeakContext.FinishTask(EStateTreeFinishTaskType::Succeeded);
			}
		});

	if (!ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(InstanceData.AbilityTag)))
	{
		ASC->OnAbilityEnded.Remove(InstanceData.AbilityEndedDelegateHandle);
		InstanceData.AbilityEndedDelegateHandle.Reset();
		return EStateTreeRunStatus::Failed;
	}

	return EStateTreeRunStatus::Running;
}

void FSTTask_FireAbility::ExitState(FStateTreeExecutionContext& Context,
									FStateTreeTransitionResult const& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (InstanceData.AbilityEndedDelegateHandle.IsValid())
	{
		if (UAbilitySystemComponent* ASC = GetASC(Context))
		{
			ASC->OnAbilityEnded.Remove(InstanceData.AbilityEndedDelegateHandle);
		}
		InstanceData.AbilityEndedDelegateHandle.Reset();
	}
}

UAbilitySystemComponent* FSTTask_FireAbility::GetASC(FStateTreeExecutionContext const& Context) const
{
	AActor const* Actor = Cast<AActor>(Context.GetOwner());
	if (!Actor)
	{
		return nullptr;
	}

	if (AController const* Controller = Cast<AController>(Actor))
	{
		Actor = Controller->GetPawn();
	}

	IAbilitySystemInterface const* AbilitySystemInterface = Cast<IAbilitySystemInterface>(Actor);
	return AbilitySystemInterface ? AbilitySystemInterface->GetAbilitySystemComponent() : nullptr;
}
