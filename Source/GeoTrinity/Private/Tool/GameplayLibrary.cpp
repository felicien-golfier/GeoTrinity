#include "Tool/GameplayLibrary.h"

#include "AbilitySystem/InteractableComponent.h"

bool GameplayLibrary::GetTeamInterface(const AActor* Actor, const IGenericTeamAgentInterface*& OutInterface)
{
	OutInterface = Cast<const IGenericTeamAgentInterface>(Actor);
	if (!OutInterface)
	{
		const UInteractableComponent* InteractableComponent = Actor->GetComponentByClass<UInteractableComponent>();
		if (IsValid(InteractableComponent))
		{
			OutInterface = Cast<const IGenericTeamAgentInterface>(InteractableComponent);
		}
	}
	return OutInterface != nullptr;
}