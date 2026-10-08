// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/Menu/GeoMenuPanelWidget.h"

#include "GeoPauseMenuWidget.generated.h"

class UGeoAbilityDescriptionsWidget;
class UGeoCharacterSheetWidget;
class UGeoGemsWidget;
class UGeoLeaderboardWidget;
class UGeoMenuButton;
class UGeoSettingsWidget;

/**
 * In-game pause menu opened via AGeoPlayerController's toggle-menu input. Owned and shown/hidden by the
 * PlayerController, not nested in a parent menu, so it exposes no OnClosed delegate.
 * Required in the BP hierarchy: UGeoMenuButton "ResumeButton", "AbilitiesButton", "SettingsButton",
 * "LeaderboardButton", "ReturnToMainMenuButton", "QuitButton", plus a UGeoAbilityDescriptionsWidget
 * "AbilitiesWidget", a UGeoSettingsWidget "SettingsWidget" and a UGeoLeaderboardWidget "LeaderboardWidget" panel
 * (all Collapsed by default). Optional: "MenuDecor", whatever dresses the top level (title, class card), which gives
 * way to a sub-panel along with the buttons; "CharacterButton" with its UGeoCharacterSheetWidget "CharacterWidget", whose
 * ABILITY DETAILS opens AbilitiesWidget in its place, closing back to it; with it, a UGeoGemsWidget "GemsWidget" its
 * GEMS opens in its place, closing back to it too.
 * The leaderboard is the same panel class the main menu opens, and it reads the save on every construct, so a run
 * finished this session shows up without leaving the level.
 */
UCLASS()
class GEOTRINITYUI_API UGeoPauseMenuWidget : public UGeoMenuPanelWidget
{
	GENERATED_BODY()

protected:
	/** Wires each pause-menu button and the SettingsWidget close delegate; collapses SettingsWidget by default. */
	virtual void NativeConstruct() override;
	/** Returns ResumeButton. */
	virtual UWidget* GetInitialFocusWidget() const override;
	/** Calls HandleResume (closes the pause menu) and consumes the back input. */
	virtual bool HandleBackAction() override;
	/** Closes the pause menu from any sub-panel depth and consumes the Escape input. */
	virtual bool HandleEscapeAction() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> ResumeButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> AbilitiesButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> SettingsButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> LeaderboardButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> ReturnToMainMenuButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> QuitButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoAbilityDescriptionsWidget> AbilitiesWidget;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoSettingsWidget> SettingsWidget;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoLeaderboardWidget> LeaderboardWidget;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> MenuDecor;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoMenuButton> CharacterButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoCharacterSheetWidget> CharacterWidget;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoGemsWidget> GemsWidget;

private:
	UFUNCTION()
	void HandleResume();

	UFUNCTION()
	void HandleAbilities();

	UFUNCTION()
	void HandleCharacter();

	/** ABILITY DETAILS on the character sheet: the abilities page, closing back to the sheet. */
	UFUNCTION()
	void HandleSheetAbilities();

	UFUNCTION()
	void HandleGems();

	UFUNCTION()
	void HandleSettings();

	UFUNCTION()
	void HandleLeaderboard();

	UFUNCTION()
	void HandleReturnToMainMenu();

	UFUNCTION()
	void HandleQuit();

	/** Back to the character sheet, refreshed, when the closed panel was opened from it; else to the top-level buttons. */
	UFUNCTION()
	void HandleSubPanelClosed();

	void OpenSubPanel(UGeoMenuPanelWidget* SubPanel);
	/** Opens SubPanel in the character sheet's place, so it closes back to the sheet. */
	void OpenFromSheet(UGeoMenuPanelWidget* SubPanel);
	/** Collapses every sub-panel. */
	void CollapseSubPanels();
	void SetButtonsVisible(bool bVisible);

	/** The open sub-panel was opened from the character sheet. */
	bool bOpenedFromSheet = false;
};
