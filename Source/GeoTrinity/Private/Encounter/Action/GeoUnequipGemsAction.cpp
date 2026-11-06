// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/Action/GeoUnequipGemsAction.h"

#include "Engine/LocalPlayer.h"
#include "GameClasses/Match/GeoPlayerState.h"
#include "Gem/GeoGemCatalog.h"
#include "Gem/GeoGemProfileSave.h"
#include "Gem/GeoGemSubsystem.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Kismet/GameplayStatics.h"

void UGeoUnequipGemsAction::Execute_Implementation(FGeoEncounterPayload const& /*Payload*/)
{
	APlayerController const* const Controller = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	AGeoPlayerState const* const PlayerState = Controller ? Controller->GetPlayerState<AGeoPlayerState>() : nullptr;
	UGeoGemSubsystem* const Gems =
		Controller ? ULocalPlayer::GetSubsystem<UGeoGemSubsystem>(Controller->GetLocalPlayer()) : nullptr;
	UGeoGemCatalog const* const Catalog = UGeoGemCatalog::Get();
	if (PlayerState && Gems && Catalog)
	{
		Gems->GetProfile()->UnequipTier(*Catalog, PlayerState->GetPlayerClass(), Tier);
		Gems->CommitChanges(PlayerState->GetPlayerClass());
	}
	else
	{
		UE_LOG(LogGeoEncounter, Warning, TEXT("%s: no local player state, gem subsystem or catalog, nothing unequipped"),
			   *GetName());
	}
}
