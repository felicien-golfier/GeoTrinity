// BTTask_FireProjectileAbility.cpp

#include "AI/Tasks/BTTask_FireProjectileAbility.h"

#include "AIController.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/EnemyCharacter.h"
#include "GameplayTagContainer.h"
#include "GameplayTagsManager.h"

UBTTask_FireProjectileAbility::UBTTask_FireProjectileAbility()
{
	NodeName = TEXT("Fire Projectile Ability");
}

EBTNodeResult::Type UBTTask_FireProjectileAbility::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (const AAIController* Controller = OwnerComp.GetAIOwner())
	{
		checkf(IsValid(Controller->GetPawn()) && Controller->GetPawn().IsA(AEnemyCharacter::StaticClass()),
			TEXT("Invalid Enemy Pawn !"));

		IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(Controller->GetPawn());
		if (!AbilitySystemInterface)
		{
			return EBTNodeResult::Failed;
		}
		UAbilitySystemComponent* ASC = AbilitySystemInterface->GetAbilitySystemComponent();
		UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
		if (!ASC || !BB)
		{
			return EBTNodeResult::Failed;
		}

		// Read name from BB and map to GameplayTag path (works for Name/String/GameplayTag keys)
		const FName TagName = BB->GetValueAsName(AbilityTagKey.SelectedKeyName);
		if (!TagName.IsNone())
		{
			const FGameplayTag AbilityTag = FGameplayTag::RequestGameplayTag(TagName, false);
			if (AbilityTag.IsValid())
			{
				const bool bActivatedByTag = ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(AbilityTag));
				return bActivatedByTag ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
			}
		}

		// No class fallback: this task relies on a Blackboard-provided gameplay tag
	}
	return EBTNodeResult::Failed;
}
