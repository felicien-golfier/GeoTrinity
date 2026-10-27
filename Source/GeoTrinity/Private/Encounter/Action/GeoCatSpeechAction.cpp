// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/Action/GeoCatSpeechAction.h"

#include "Characters/GeoCharacter.h"
#include "Encounter/GeoEncounter.h"
#include "Encounter/GeoEncounterSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HUD/Interface/GeoHUDInterface.h"
#include "Tool/UGeoGameplayLibrary.h"

void UGeoCatSpeechAction::Execute_Implementation(FGeoEncounterPayload const& /*Payload*/)
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
		Player->OnRevived.AddUObject(this, &UGeoCatSpeechAction::ShowLine);
	}
	else
	{
		ShowLine();
	}
}

void UGeoCatSpeechAction::ShowLine()
{
	if (AGeoCharacter* const Player = WaitedPlayer.Get())
	{
		Player->OnRevived.RemoveAll(this);
	}
	WaitedPlayer.Reset();

	FText const RawLine = GetLineText();
	if (RawLine.IsEmpty())
	{
		Finish();
	}
	else
	{
		FText const ShownLine = FText::FromString(GeoLib::ReplaceCounterTokens(RawLine.ToString(), GetEncounter()));
		GetWorld()->GetSubsystem<UGeoEncounterSubsystem>()->OnCatLine.Broadcast(ShownLine);

		if (IGeoHUDInterface* const HUD = GetLocalHUD())
		{
			bLineShowing = true;
			HUD->ShowCatLine(ShownLine,
							 FSimpleDelegate::CreateWeakLambda(this,
															   [this, Run = LineRun, Shown = ++ShownLines]()
															   {
																   if (Shown == ShownLines)
																   {
																	   bLineShowing = false;
																   }

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
}

void UGeoCatSpeechAction::OnCancel_Implementation()
{
	if (AGeoCharacter* const Player = WaitedPlayer.Get())
	{
		Player->OnRevived.RemoveAll(this);
	}
	WaitedPlayer.Reset();
	++LineRun;

	if (bLineShowing && GetEncounter()->IsAborting())
	{
		if (IGeoHUDInterface* const HUD = GetLocalHUD())
		{
			HUD->HideCatLine();
		}
	}
}
