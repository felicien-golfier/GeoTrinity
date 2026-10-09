// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/Menu/GeoMenuPageWidget.h"

#include "GeoSettingsWidget.generated.h"

class UGeoMenuButton;

/**
 * Settings chooser page: opens the Sound, Key Bindings or Interface page of its menu, each a page of its own whose Back
 * returns here.
 * Required in the BP hierarchy: UGeoMenuButton "SoundButton", "KeyBindingsButton", "InterfaceButton".
 */
UCLASS()
class GEOTRINITYUI_API UGeoSettingsWidget : public UGeoMenuPageWidget
{
	GENERATED_BODY()

protected:
	/** Wires the chooser buttons. */
	virtual void NativeConstruct() override;
	/** Returns SoundButton. */
	virtual UWidget* GetInitialFocusWidget() const override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> SoundButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> KeyBindingsButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> InterfaceButton;

private:
	UFUNCTION()
	void HandleSound();

	UFUNCTION()
	void HandleKeyBindings();

	UFUNCTION()
	void HandleInterface();
};
