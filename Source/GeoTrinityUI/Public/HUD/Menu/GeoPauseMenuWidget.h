// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/Menu/GeoMenuPanelWidget.h"

#include "GeoPauseMenuWidget.generated.h"

class UGeoAbilityDescriptionsWidget;
class UGeoMenuButton;
class UGeoSettingsWidget;

/**
 * In-game pause menu opened via AGeoPlayerController's toggle-menu input. Owned and shown/hidden by the
 * PlayerController, not nested in a parent menu, so it exposes no OnClosed delegate.
 * Required in the BP hierarchy: UGeoMenuButton "ResumeButton", "AbilitiesButton", "SettingsButton",
 * "ReturnToMainMenuButton", "QuitButton", plus a UGeoAbilityDescriptionsWidget "AbilitiesWidget" and a
 * UGeoSettingsWidget "SettingsWidget" panel (both Collapsed by default).
 */
UCLASS()
class GEOTRINITYUI_API UGeoPauseMenuWidget : public UGeoMenuPanelWidget
{
	GENERATED_BODY()

protected:
	/** Wires each pause-menu button and the SettingsWidget close delegate; collapses SettingsWidget by default. */
	virtual void NativeConstruct() override;
	/** Removes all button and sub-panel delegates bound in NativeConstruct. */
	virtual void NativeDestruct() override;
	/** Returns ResumeButton. */
	virtual UWidget* GetInitialFocusWidget() const override;
	/** Calls HandleResume (closes the pause menu) and consumes the back input. */
	virtual bool HandleBackAction() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> ResumeButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> AbilitiesButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> SettingsButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> ReturnToMainMenuButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> QuitButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoAbilityDescriptionsWidget> AbilitiesWidget;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoSettingsWidget> SettingsWidget;

private:
	UFUNCTION()
	void HandleResume();

	UFUNCTION()
	void HandleAbilities();

	UFUNCTION()
	void HandleSettings();

	UFUNCTION()
	void HandleReturnToMainMenu();

	UFUNCTION()
	void HandleQuit();

	UFUNCTION()
	void HandleSubPanelClosed();

	void OpenSubPanel(UGeoMenuPanelWidget* SubPanel);
	void SetButtonsVisible(bool bVisible);
};
