// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Team/GeoXpStripWidget.h"

#include "Components/TextBlock.h"
#include "Engine/LocalPlayer.h"
#include "GameClasses/Match/GeoPlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Gem/GeoGemCatalog.h"
#include "Gem/GeoGemProfileSave.h"
#include "Gem/GeoGemSubsystem.h"
#include "HUD/Style/Meter/GeoMeter.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoXpStripWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (LevelText)
	{
		LevelText->SetColorAndOpacity(FSlateColor(LevelColor));
	}
	if (EchoLevelText)
	{
		EchoLevelText->SetColorAndOpacity(FSlateColor(EchoLevelColor));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoXpStripWidget::NativeDestruct()
{
	if (UGeoGemSubsystem* Subsystem = GemSubsystem.Get())
	{
		Subsystem->OnProfileCommitted.RemoveAll(this);
	}

	Super::NativeDestruct();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoXpStripWidget::InitForPlayer(AGeoPlayerState* InPlayerState)
{
	if (!ensureMsgf(InPlayerState, TEXT("%hs: no player state on %s"), __FUNCTION__, *GetName()))
	{
		return;
	}

	PlayerState = InPlayerState;
	APlayerController const* PlayerController = InPlayerState->GetPlayerController();
	ULocalPlayer const* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	GemSubsystem = ULocalPlayer::GetSubsystem<UGeoGemSubsystem>(LocalPlayer);
	if (UGeoGemSubsystem* Subsystem = GemSubsystem.Get())
	{
		Subsystem->OnProfileCommitted.AddUObject(this, &UGeoXpStripWidget::Refresh);
	}

	Refresh();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoXpStripWidget::Refresh()
{
	AGeoPlayerState const* ShownPlayer = PlayerState.Get();
	UGeoGemSubsystem const* Subsystem = GemSubsystem.Get();
	UGeoGemCatalog const* Catalog = UGeoGemCatalog::Get();
	if (ShownPlayer && Subsystem && Catalog)
	{
		UGeoGemProfileSave const& Profile = *Subsystem->GetProfile();
		EPlayerClass const PlayerClass = ShownPlayer->GetPlayerClass();
		int32 XpTaken = 0;
		int32 XpNeeded = 0;
		float const Fill = Profile.GetXpProgress(*Catalog, PlayerClass, XpTaken, XpNeeded);
		if (XpMeter)
		{
			XpMeter->SetFill(Fill);
		}
		if (LevelText)
		{
			LevelText->SetText(FText::AsNumber(Profile.GetClassLevel(PlayerClass)));
		}
		if (EchoLevelText)
		{
			EchoLevelText->SetText(FText::Format(EchoLevelFormat, Profile.GetEchoLevel()));
			EchoLevelText->SetVisibility(Profile.GetEchoLevel() >= 1 ? ESlateVisibility::HitTestInvisible
																	 : ESlateVisibility::Hidden);
		}
	}
}
