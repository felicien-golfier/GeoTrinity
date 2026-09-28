// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/Menu/GeoMenuPanelWidget.h"

#include "GeoSoundSettingsWidget.generated.h"

class UGeoMenuButton;
class USlider;
enum class EGeoVolumeChannel : uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGeoSoundSettingsClosedSignature);

/**
 * Sound settings panel: one slider per EGeoVolumeChannel, each read from and written straight to
 * UGeoGameUserSettings, which applies and saves it at once.
 * Communicates back to the parent menu exclusively via the OnClosed delegate.
 * Required in the BP hierarchy: UGeoMenuButton "BackButton", USlider "GeneralVolumeSlider", "EffectsVolumeSlider",
 * "MusicVolumeSlider", "InterfaceVolumeSlider".
 */
UCLASS()
class GEOTRINITYUI_API UGeoSoundSettingsWidget : public UGeoMenuPanelWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "GeoMenu")
	FGeoSoundSettingsClosedSignature OnClosed;

protected:
	/** Wires BackButton and the volume sliders, and sets each slider to its saved volume. */
	virtual void NativeConstruct() override;
	/** Returns BackButton. */
	virtual UWidget* GetInitialFocusWidget() const override;
	/** Fires OnClosed and consumes the back input. */
	virtual bool HandleBackAction() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> BackButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<USlider> GeneralVolumeSlider;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<USlider> EffectsVolumeSlider;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<USlider> MusicVolumeSlider;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<USlider> InterfaceVolumeSlider;

private:
	UFUNCTION()
	void HandleBack();

	UFUNCTION()
	void HandleGeneralVolumeChanged(float Value);

	UFUNCTION()
	void HandleEffectsVolumeChanged(float Value);

	UFUNCTION()
	void HandleMusicVolumeChanged(float Value);

	UFUNCTION()
	void HandleInterfaceVolumeChanged(float Value);

	/** Stores and saves Volume, then applies every volume to this widget's audio device. */
	void SetVolume(EGeoVolumeChannel Channel, float Volume) const;
};
