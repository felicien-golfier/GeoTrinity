// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/Menu/GeoMenuPanelWidget.h"

#include "GeoSettingsWidget.generated.h"

class UGeoKeyBindingsWidget;
class UGeoMenuButton;
class UGeoSoundSettingsWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGeoSettingsClosedSignature);

/**
 * Settings chooser layer: opens the Sound or Key Bindings sub-panel, each a separate window shown while the
 * chooser buttons hide. Communicates back to the parent menu exclusively via the OnClosed delegate.
 * Required in the BP hierarchy: UGeoMenuButton "SoundButton", "KeyBindingsButton", "BackButton", plus a
 * UGeoSoundSettingsWidget "SoundWidget" and a UGeoKeyBindingsWidget "KeyBindingsWidget" (Collapsed by default).
 */
UCLASS()
class GEOTRINITYUI_API UGeoSettingsWidget : public UGeoMenuPanelWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Menu")
	FGeoSettingsClosedSignature OnClosed;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual UWidget* GetInitialFocusWidget() const override;
	virtual bool HandleBackAction() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> SoundButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> KeyBindingsButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> BackButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoSoundSettingsWidget> SoundWidget;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoKeyBindingsWidget> KeyBindingsWidget;

private:
	UFUNCTION()
	void HandleSound();

	UFUNCTION()
	void HandleKeyBindings();

	UFUNCTION()
	void HandleBack();

	UFUNCTION()
	void HandleSubPanelClosed();

	void OpenSubPanel(UGeoMenuPanelWidget* SubPanel);
	void SetButtonsVisible(bool bVisible);
};
