// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/GeoOverlayWidget.h"

#include "Characters/PlayableCharacter.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "HUD/GeoAbilityBarWidget.h"
#include "HUD/GeoStatusBarWidget.h"

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

	// Player 1's overlay is built long before anyone presses Start, and a column is only as wide as the player count
	// makes it, so the layout has to follow players joining and leaving.
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		GameInstance->OnLocalPlayerAddedEvent.AddWeakLambda(this, [this](ULocalPlayer*) { ApplyColumnLayout(); });
		GameInstance->OnLocalPlayerRemovedEvent.AddWeakLambda(this, [this](ULocalPlayer*) { ApplyColumnLayout(); });
	}
	ApplyColumnLayout();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoOverlayWidget::NativeDestruct()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		GameInstance->OnLocalPlayerAddedEvent.RemoveAll(this);
		GameInstance->OnLocalPlayerRemovedEvent.RemoveAll(this);
	}
	Super::NativeDestruct();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoOverlayWidget::ApplyColumnLayout()
{
	UCanvasPanel const* RootCanvas = Cast<UCanvasPanel>(GetRootWidget());
	UGameInstance const* GameInstance = GetGameInstance();
	ULocalPlayer const* LocalPlayer = GetOwningLocalPlayer();
	// INDEX_NONE covers the removal broadcast reaching the overlay of the player that was just dropped.
	int32 const Column = LocalPlayer ? LocalPlayer->GetLocalPlayerIndex() : INDEX_NONE;
	if (!RootCanvas || !GameInstance || Column == INDEX_NONE)
	{
		return;
	}

	int32 const ColumnCount = GameInstance->GetNumLocalPlayers();
	for (UWidget* Child : RootCanvas->GetAllChildren())
	{
		UCanvasPanelSlot* const CanvasSlot = Child ? Cast<UCanvasPanelSlot>(Child->Slot) : nullptr;
		if (!CanvasSlot)
		{
			continue;
		}

		// Undo the column the anchors are standing in to recover the authored full-width ones, then squeeze those into
		// the new column, so a third player joining moves everyone from halves to thirds instead of compounding.
		FAnchors Anchors = CanvasSlot->GetAnchors();
		Anchors.Minimum.X = (Anchors.Minimum.X * AppliedColumnCount - AppliedColumn + Column) / ColumnCount;
		Anchors.Maximum.X = (Anchors.Maximum.X * AppliedColumnCount - AppliedColumn + Column) / ColumnCount;
		CanvasSlot->SetAnchors(Anchors);
	}

	AppliedColumn = Column;
	AppliedColumnCount = ColumnCount;
}
