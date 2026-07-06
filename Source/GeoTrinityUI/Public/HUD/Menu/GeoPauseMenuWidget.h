// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "GeoPauseMenuWidget.generated.h"

class UGeoMenuButton;
class UGeoSettingsWidget;

/**
 * In-game pause menu opened via AGeoPlayerController's toggle-menu input. Owned and shown/hidden by the
 * PlayerController, not nested in a parent menu, so it exposes no OnClosed delegate.
 * Required in the BP hierarchy: UGeoMenuButton "ResumeButton", "SettingsButton", "ReturnToMainMenuButton",
 * "QuitButton", and a UGeoSettingsWidget "SettingsWidget" panel (Collapsed by default).
 */
UCLASS()
class GEOTRINITYUI_API UGeoPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> ResumeButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> SettingsButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> ReturnToMainMenuButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> QuitButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoSettingsWidget> SettingsWidget;

private:
	UFUNCTION()
	void HandleResume();

	UFUNCTION()
	void HandleSettings();

	UFUNCTION()
	void HandleReturnToMainMenu();

	UFUNCTION()
	void HandleQuit();

	UFUNCTION()
	void HandleSettingsClosed();

	void SetButtonsVisible(bool bVisible);
};
