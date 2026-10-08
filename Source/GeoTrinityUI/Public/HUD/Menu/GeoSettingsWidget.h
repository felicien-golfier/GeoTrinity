// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/Menu/GeoMenuPanelWidget.h"

#include "GeoSettingsWidget.generated.h"

class UGeoInterfaceSettingsWidget;
class UGeoKeyBindingsWidget;
class UGeoMenuButton;
class UGeoSoundSettingsWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGeoSettingsClosedSignature);

/**
 * Settings chooser layer: opens the Sound, Key Bindings or Interface sub-panel, each a separate window shown while
 * the chooser buttons hide. Communicates back to the parent menu exclusively via the OnClosed delegate.
 * Required in the BP hierarchy: UGeoMenuButton "SoundButton", "KeyBindingsButton", "InterfaceButton", "BackButton",
 * plus a UGeoSoundSettingsWidget "SoundWidget", a UGeoKeyBindingsWidget "KeyBindingsWidget" and a
 * UGeoInterfaceSettingsWidget "InterfaceWidget" (Collapsed by default).
 */
UCLASS()
class GEOTRINITYUI_API UGeoSettingsWidget : public UGeoMenuPanelWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "GeoMenu")
	FGeoSettingsClosedSignature OnClosed;

protected:
	/** Wires the chooser buttons and the sub-panel close delegates. */
	virtual void NativeConstruct() override;
	/** Returns SoundButton. */
	virtual UWidget* GetInitialFocusWidget() const override;
	/** Fires OnClosed and consumes the back input. */
	virtual bool HandleBackAction() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> SoundButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> KeyBindingsButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> InterfaceButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> BackButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoSoundSettingsWidget> SoundWidget;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoKeyBindingsWidget> KeyBindingsWidget;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoInterfaceSettingsWidget> InterfaceWidget;

private:
	UFUNCTION()
	void HandleSound();

	UFUNCTION()
	void HandleKeyBindings();

	UFUNCTION()
	void HandleInterface();

	UFUNCTION()
	void HandleBack();

	UFUNCTION()
	void HandleSubPanelClosed();

	void OpenSubPanel(UGeoMenuPanelWidget* SubPanel);
	void SetButtonsVisible(bool bVisible);
};
