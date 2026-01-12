#include "Tool/GameplayLibrary.h"

bool GameplayLibrary::GetTeamInterface(const AActor* Actor, const IGenericTeamAgentInterface*& OutInterface)
{
	OutInterface = Cast<const IGenericTeamAgentInterface>(Actor);
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
double GameplayLibrary::GetTime()
{
	return FPlatformTime::Seconds() - GStartTime;
}