// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/Action/GeoUnlockAbilityAction.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Characters/PlayableCharacter.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GeoTrinity/GeoTrinity.h"
#include "HUD/Interface/GeoHUDInterface.h"

void UGeoUnlockAbilityAction::Execute_Implementation(FGeoEncounterPayload const& /*Payload*/)
{
	IGeoHUDInterface* const HUD = GrantAbility() ? GetLocalHUD() : nullptr;
	if (HUD)
	{
		HUD->ShowAbilityPage(AbilityTag);
	}
}

void UGeoUnlockAbilityAction::OnSkipped_Implementation()
{
	GrantAbility();
}

bool UGeoUnlockAbilityAction::GrantAbility() const
{
	APlayerController const* const PlayerController = GetWorld()->GetFirstPlayerController();
	APlayableCharacter* const Player =
		PlayerController ? Cast<APlayableCharacter>(PlayerController->GetPawn()) : nullptr;
	UGeoAbilitySystemComponent* const ASC = GeoASLib::GetGeoAscFromActor(Player);
	if (!ASC)
	{
		UE_LOG(LogGeoEncounter, Warning, TEXT("%s: no local player to unlock %s for"), *GetName(),
			   *AbilityTag.ToString());
	}

	return ASC && ASC->GiveClassAbility(Player->GetPlayerClass(), AbilityTag);
}
