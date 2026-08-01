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
 *
 * In couch coop every local player's overlay is added to the same viewport, so the authored solo layout would stack
 * them on top of each other; ApplyColumnLayout gives each player their own column.
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
	/** Lays the overlay out for its own player, and re-runs whenever a local player joins or leaves. */
	virtual void NativeConstruct() override;
	/** Unregisters local-player add/remove delegates so the layout callback cannot fire on a destroyed widget. */
	virtual void NativeDestruct() override;

	/** Bottom-center ability bar. Bound from WBP_MainOverlay; rebuilt by the HUD when abilities are granted/changed. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoAbilityBarWidget> AbilityBar;

	/** Active-effect icon row, bound from WBP_MainOverlay above the ability bar. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoStatusBarWidget> StatusBar;

private:
	/**
	 * Squeezes the root canvas's children into this player's column of the shared view — player 1 left, player 2
	 * middle, player 3 right — keeping the authored layout inside it. With a single local player the columns are the
	 * whole screen, so solo is exactly as authored.
	 */
	void ApplyColumnLayout();

	/** Column the anchors currently sit in, so a re-layout recovers the authored ones rather than compounding. */
	int32 AppliedColumn = 0;

	/** Number of columns the anchors were divided into; 1 means they are still exactly as authored. */
	int32 AppliedColumnCount = 1;
};
