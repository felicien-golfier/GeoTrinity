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

FColor GameplayLibrary::GetColorForObject(const UObject* Object)
{
	if (!IsValid(Object))
	{
		return FColor::White;
	}

	static const FColor Palette[] = {FColor::Black, FColor::Red, FColor::Green, FColor::Blue, FColor::Yellow,
		FColor::Cyan, FColor::Magenta, FColor::Orange, FColor::Emerald, FColor::Purple, FColor::Turquoise,
		FColor::Silver};

	return Palette[Object->GetUniqueID() % std::size(Palette)];
}