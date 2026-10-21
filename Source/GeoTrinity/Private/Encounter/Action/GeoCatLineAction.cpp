// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/Action/GeoCatLineAction.h"

#include "Characters/GeoCharacter.h"
#include "Encounter/GeoEncounter.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HUD/Interface/GeoHUDInterface.h"
#include "Tool/UGeoGameplayLibrary.h"

void UGeoCatLineAction::Execute_Implementation(FGeoEncounterPayload const& /*Payload*/)
{
	if (AGeoCharacter* const Previous = WaitedPlayer.Get())
	{
		Previous->OnRevived.RemoveAll(this);
	}
	WaitedPlayer.Reset();

	APlayerController const* const PlayerController = GetWorld()->GetFirstPlayerController();
	AGeoCharacter* const Player = PlayerController ? Cast<AGeoCharacter>(PlayerController->GetPawn()) : nullptr;
	if (bWaitForRespawn && Player && Player->IsDead())
	{
		WaitedPlayer = Player;
		Player->OnRevived.AddUObject(this, &UGeoCatLineAction::ShowLine);
	}
	else
	{
		ShowLine();
	}
}

void UGeoCatLineAction::ShowLine()
{
	if (AGeoCharacter* const Player = WaitedPlayer.Get())
	{
		Player->OnRevived.RemoveAll(this);
	}
	WaitedPlayer.Reset();

	if (IGeoHUDInterface* const HUD = GetLocalHUD())
	{
		HUD->ShowCatLine(FText::FromString(GeoLib::ReplaceCounterTokens(Line.ToString(), GetEncounter())),
						 FSimpleDelegate::CreateWeakLambda(this,
														   [this, Run = LineRun]()
														   {
															   if (Run == LineRun)
															   {
																   Finish();
															   }
														   }));
	}
	else
	{
		Finish();
	}
}

void UGeoCatLineAction::OnCancel_Implementation()
{
	if (AGeoCharacter* const Player = WaitedPlayer.Get())
	{
		Player->OnRevived.RemoveAll(this);
	}
	WaitedPlayer.Reset();
	++LineRun;
}
