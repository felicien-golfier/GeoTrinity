// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/GeoOverlayWidget.h"

#include "Characters/PlayableCharacter.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "HUD/GeoAbilityBarWidget.h"
#include "HUD/GeoStatusBarWidget.h"

namespace
{
	// Anchor-space gap left between the screen centre line and the inner edge of a player's centred HUD elements.
	constexpr float CenterGap = 0.02f;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoOverlayWidget::BuildAbilityBar(AGeoHUD* GeoHUD, APlayableCharacter* PlayableCharacter)
{
	if (!ensureMsgf(AbilityBar, TEXT("UGeoOverlayWidget::BuildAbilityBar — AbilityBar is not bound on %s"), *GetName()))
	{
		return;
	}

	AbilityBar->BuildBar(GeoHUD, PlayableCharacter);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoOverlayWidget::InitStatusBar(AGeoHUD* GeoHUD)
{
	if (!ensureMsgf(StatusBar, TEXT("UGeoOverlayWidget::InitStatusBar — StatusBar is not bound on %s"), *GetName()))
	{
		return;
	}

	StatusBar->InitStatusBar(GeoHUD);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoOverlayWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Player 1's overlay is built long before anyone presses Start, so it has to re-lay-out when they do.
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		GameInstance->OnLocalPlayerAddedEvent.AddWeakLambda(this,
															[this](ULocalPlayer*)
															{
																ApplySideLayout();
															});
	}
	ApplySideLayout();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoOverlayWidget::NativeDestruct()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		GameInstance->OnLocalPlayerAddedEvent.RemoveAll(this);
	}
	Super::NativeDestruct();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoOverlayWidget::ApplySideLayout()
{
	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(GetRootWidget());
	UGameInstance const* GameInstance = GetGameInstance();
	ULocalPlayer const* LocalPlayer = GetOwningLocalPlayer();
	if (bSideLayoutApplied || !RootCanvas || !GameInstance || !LocalPlayer || GameInstance->GetNumLocalPlayers() <= 1)
	{
		return;
	}
	bSideLayoutApplied = true;

	bool const bMirror = LocalPlayer->GetLocalPlayerIndex() % 2 == 1;
	for (UWidget* Child : RootCanvas->GetAllChildren())
	{
		UCanvasPanelSlot* CanvasSlot = Child ? Cast<UCanvasPanelSlot>(Child->Slot) : nullptr;
		if (!CanvasSlot)
		{
			continue;
		}

		FAnchors Anchors = CanvasSlot->GetAnchors();
		FMargin Offsets = CanvasSlot->GetOffsets();
		float const AnchorWidth = Anchors.Maximum.X - Anchors.Minimum.X;
		if (Anchors.Minimum.X < 0.5f && Anchors.Maximum.X > 0.5f)
		{
			Anchors.Maximum.X = 0.5f - CenterGap;
			Anchors.Minimum.X = Anchors.Maximum.X - AnchorWidth;
		}
		else if (Anchors.Minimum.X >= 0.5f)
		{
			float const NormalisedMinX = 1.f - Anchors.Maximum.X;
			Anchors.Maximum.X = 1.f - Anchors.Minimum.X;
			Anchors.Minimum.X = NormalisedMinX;
			Swap(Offsets.Left, Offsets.Right);
		}

		if (bMirror)
		{
			float const MirroredMinX = 1.f - Anchors.Maximum.X;
			Anchors.Maximum.X = 1.f - Anchors.Minimum.X;
			Anchors.Minimum.X = MirroredMinX;
			Swap(Offsets.Left, Offsets.Right);
		}

		CanvasSlot->SetAnchors(Anchors);
		CanvasSlot->SetOffsets(Offsets);
	}
}
