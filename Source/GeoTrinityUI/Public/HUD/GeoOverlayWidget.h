// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Characters/PlayableCharacter.h"
#include "CoreMinimal.h"
#include "HUD/GeoUserWidget.h"

#include "GeoOverlayWidget.generated.h"

class UGeoAbilityBarWidget;
class UGeoStatusBarWidget;
class AGeoHUD;

/**
 * Root player overlay widget. Holds the bottom-center ability bar and status bar as BindWidgets so the HUD can drive
 * them directly from C++ (AGeoHUD::BuildAbilityBar, AGeoHUD::InitOverlay) without any Blueprint event-graph wiring.
 * WBP_MainOverlay reparents to this class and places a WBP_AbilityBar named "AbilityBar" and a WBP_StatusBar named
 * "StatusBar" anchored bottom-center.
 */
UCLASS()
class GEOTRINITYUI_API UGeoOverlayWidget : public UGeoUserWidget
{
	GENERATED_BODY()

public:
	/** Rebuilds the ability bar from the HUD's current ability set. Called by AGeoHUD::BuildAbilityBar. */
	void BuildAbilityBar(AGeoHUD* GeoHUD, APlayableCharacter* PlayableCharacter);

	/** Gives the status bar the HUD reference it polls for active effect icons. Called by AGeoHUD::InitOverlay. */
	void InitStatusBar(AGeoHUD* GeoHUD);

protected:
	/** Bottom-center ability bar. Bound from WBP_MainOverlay; rebuilt by the HUD when abilities are granted/changed. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoAbilityBarWidget> AbilityBar;

	/** Active-effect icon row, bound from WBP_MainOverlay above the ability bar. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoStatusBarWidget> StatusBar;
};
