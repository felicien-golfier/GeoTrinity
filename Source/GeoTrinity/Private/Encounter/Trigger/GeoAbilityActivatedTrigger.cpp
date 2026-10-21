// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/Trigger/GeoAbilityActivatedTrigger.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GeoTrinity/GeoTrinity.h"

void UGeoAbilityActivatedTrigger::OnArm_Implementation()
{
	Activations = 0;
	APlayerController const* const PlayerController = GetWorld()->GetFirstPlayerController();
	WatchedASC = GeoASLib::GetGeoAscFromActor(PlayerController ? PlayerController->GetPawn() : nullptr);
	if (UGeoAbilitySystemComponent* const ASC = WatchedASC.Get())
	{
		ASC->AbilityActivatedCallbacks.AddUObject(this, &UGeoAbilityActivatedTrigger::OnAbilityActivated);
	}
	else
	{
		UE_LOG(LogGeoEncounter, Warning, TEXT("%s: no local player to watch"), *GetName());
	}
}

void UGeoAbilityActivatedTrigger::OnDisarm_Implementation()
{
	if (UGeoAbilitySystemComponent* const ASC = WatchedASC.Get())
	{
		ASC->AbilityActivatedCallbacks.RemoveAll(this);
	}
}

void UGeoAbilityActivatedTrigger::OnAbilityActivated(UGameplayAbility* Ability)
{
	if (GeoASLib::GetAbilityTagFromAbility(*Ability) == AbilityTag)
	{
		FGeoEncounterPayload Payload;
		Payload.Actor = WatchedASC->GetAvatarActor();
		Payload.Count = ++Activations;
		Fire(Payload);
	}
}
