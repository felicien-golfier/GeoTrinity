// BTTask_FireProjectileAbility.cpp

#include "AI/Tasks/BTTask_FireProjectileAbility.h"

#include "AIController.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystem/InteractableComponent.h"
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
		AEnemyCharacter* Enemy = CastChecked<AEnemyCharacter>(Controller->GetPawn());

		UInteractableComponent* Interactable = Enemy->FindComponentByClass<UInteractableComponent>();
		if (UGeoAbilitySystemComponent* ASC = Interactable ? Interactable->AbilitySystemComponent : nullptr)
		{
			// Try activate by gameplay tag from Blackboard first (if provided)
			if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
			{
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
			}

			// No class fallback: this task relies on a Blackboard-provided gameplay tag
		}
	}
	return EBTNodeResult::Failed;
}
