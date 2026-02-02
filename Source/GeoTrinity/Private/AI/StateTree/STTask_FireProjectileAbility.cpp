// STTask_FireProjectileAbility.cpp

#include "AI/StateTree/STTask_FireProjectileAbility.h"

#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "StateTreeExecutionContext.h"
#include "Characters/EnemyCharacter.h"

EStateTreeRunStatus FSTTask_FireProjectileAbility::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	AActor const* Actor = Cast<AActor>(Context.GetOwner());
	if (!Actor)
	{
		return EStateTreeRunStatus::Failed;
	}

	// If owner is a controller, get the pawn
	if (AController const* Controller = Cast<AController>(Actor))
	{
		Actor = Controller->GetPawn();
	}

	if (!Actor || !Actor->IsA(AEnemyCharacter::StaticClass()))
	{
		return EStateTreeRunStatus::Failed;
	}

	IAbilitySystemInterface const* AbilitySystemInterface = Cast<IAbilitySystemInterface>(Actor);
	if (!AbilitySystemInterface)
	{
		return EStateTreeRunStatus::Failed;
	}

	UAbilitySystemComponent* ASC = AbilitySystemInterface->GetAbilitySystemComponent();
	if (!ASC || !InstanceData.AbilityTag.IsValid())
	{
		return EStateTreeRunStatus::Failed;
	}

	bool const bActivated = ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(InstanceData.AbilityTag));
	return bActivated ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Failed;
}
