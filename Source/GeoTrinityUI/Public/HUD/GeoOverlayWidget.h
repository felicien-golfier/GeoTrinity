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
 * them on top of each other; ApplySideLayout gives each player their own half.
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
	/** Lays the overlay out for its own player, and re-runs once a second local player joins. */
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Bottom-center ability bar. Bound from WBP_MainOverlay; rebuilt by the HUD when abilities are granted/changed. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoAbilityBarWidget> AbilityBar;

	/** Active-effect icon row, bound from WBP_MainOverlay above the ability bar. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoStatusBarWidget> StatusBar;

private:
	/**
	 * Moves the root canvas's children onto this player's half of the shared view, keeping their authored anchor
	 * widths: elements straddling the centre line tuck against it, right-anchored ones normalise to the left, then
	 * player 2's whole set mirrors to the right. No-op while there is a single local player, so solo is exactly as
	 * authored. Applied at most once — it reads the authored anchors, so a second pass would compound.
	 */
	void ApplySideLayout();

	bool bSideLayoutApplied = false;
};
