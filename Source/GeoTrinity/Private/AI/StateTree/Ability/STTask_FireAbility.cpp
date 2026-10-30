// STTask_FireProjectileAbility.cpp

#include "AI/StateTree/Ability/STTask_FireAbility.h"

#include "Abilities/GameplayAbility.h"
#include "AbilitySystem/Abilities/Base/PatternAbilityParams.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Actor/Arena/GeoArena.h"
#include "Engine/World.h"
#include "StateTreeAsyncExecutionContext.h"
#include "StateTreeExecutionContext.h"
#include "TimerManager.h"
#include "Tool/UGeoGameplayLibrary.h"

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
		[WeakContext = Context.MakeWeakExecutionContext(), Tag = InstanceData.AbilityTag,
		 Delay = InstanceData.DelayAfterAbilityEnd, World = Context.GetWorld()](FAbilityEndedData const& EndedData)
		{
			if (EndedData.AbilityThatEnded && EndedData.AbilityThatEnded->GetAssetTags().HasTag(Tag))
			{
				if (Delay > 0.f)
				{
					FTimerHandle TimerHandle;
					World->GetTimerManager().SetTimer(
						TimerHandle,
						[WeakContext]()
						{
							WeakContext.FinishTask(EStateTreeFinishTaskType::Succeeded);
						},
						Delay, false);
				}
				else
				{
					WeakContext.FinishTask(EStateTreeFinishTaskType::Succeeded);
				}
			}
		});

	if (!TryActivate(Context, *ASC, InstanceData))
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

bool FSTTask_FireAbility::TryActivate(FStateTreeExecutionContext const& Context, UAbilitySystemComponent& ASC,
									  FInstanceDataType const& InstanceData) const
{
	FGameplayTagContainer const AbilityTags(InstanceData.AbilityTag);
	if (!InstanceData.PatternData.IsValid())
	{
		return ASC.TryActivateAbilitiesByTag(AbilityTags);
	}

	TArray<FGameplayAbilitySpec*> Specs;
	ASC.GetActivatableGameplayAbilitySpecsByAllMatchingTags(AbilityTags, Specs);
	if (Specs.IsEmpty())
	{
		return false;
	}

	FPatternAbilityParams* const Params = new FPatternAbilityParams();
	if (AGeoArena const* const Arena = AGeoArena::GetArenaOfBoss(GeoLib::ResolveOwnerPawn(Context.GetOwner())))
	{
		Params->ArenaTag = Arena->ArenaTag;
	}

	Params->PatternData = InstanceData.PatternData;
	FGameplayEventData ActivationData;
	ActivationData.TargetData.Add(Params);
	return ASC.InternalTryActivateAbility(Specs[0]->Handle, FPredictionKey(), nullptr, nullptr, &ActivationData);
}

UAbilitySystemComponent* FSTTask_FireAbility::GetASC(FStateTreeExecutionContext const& Context) const
{
	IAbilitySystemInterface const* AbilitySystemInterface =
		Cast<IAbilitySystemInterface>(GeoLib::ResolveOwnerPawn(Context.GetOwner()));
	return AbilitySystemInterface ? AbilitySystemInterface->GetAbilitySystemComponent() : nullptr;
}
