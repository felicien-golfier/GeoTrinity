// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/Action/GeoCatSpeechAction.h"

#include "Characters/GeoCharacter.h"
#include "Encounter/GeoEncounter.h"
#include "Encounter/GeoEncounterSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HUD/Interface/GeoHUDInterface.h"
#include "Misc/DataValidation.h"
#include "Tool/CatLine/GeoCatLineBank.h"
#include "Tool/UGeoGameplayLibrary.h"

#if WITH_EDITOR
EDataValidationResult UGeoCatSpeechAction::IsDataValid(FDataValidationContext& Context) const
{
	Super::IsDataValid(Context);

	if (bTimed)
	{
		float const LifeSeconds = Seconds.GetValueAtLevel(0.f);
		float const TypingSeconds = GetLongestLineText().ToString().Len() / GeoCatLines::TypingCharsPerSecond;
		if (LifeSeconds <= 0.f)
		{
			Context.AddError(FText::FromString(FString::Printf(TEXT("%s: timed line without Seconds"), *GetName())));
		}
		else if (TypingSeconds > LifeSeconds)
		{
			Context.AddError(FText::FromString(FString::Printf(
				TEXT("%s: the line needs %.1f s to type, Seconds is %.1f"), *GetName(), TypingSeconds, LifeSeconds)));
		}
	}

	return Context.GetNumErrors() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif

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
			float const TimedSeconds = bTimed ? Seconds.GetValueAtLevel(0.f) : 0.f;
			HUD->ShowCatLine(ShownLine,
							 FSimpleDelegate::CreateWeakLambda(this,
															   [this, Run = LineRun, Shown = ++ShownLines]()
															   {
																   if (Shown == ShownLines)
																   {
																	   bLineShowing = false;
																   }

																   if (Run == LineRun && !bTimed)
																   {
																	   Finish();
																   }
															   }),
							 TimedSeconds);
			if (bTimed)
			{
				Finish();
			}
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
