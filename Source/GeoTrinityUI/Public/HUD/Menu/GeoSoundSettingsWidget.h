// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/Menu/GeoMenuPageWidget.h"

#include "GeoSoundSettingsWidget.generated.h"

class USlider;
enum class EGeoVolumeChannel : uint8;

/**
 * Sound settings panel: one slider per EGeoVolumeChannel, each read from and written straight to
 * UGeoGameUserSettings, which applies and saves it at once.
 * Required in the BP hierarchy: USlider "GeneralVolumeSlider", "EffectsVolumeSlider",
 * "MusicVolumeSlider", "InterfaceVolumeSlider".
 */
UCLASS()
class GEOTRINITYUI_API UGeoSoundSettingsWidget : public UGeoMenuPageWidget
{
	GENERATED_BODY()

protected:
	/** Wires the volume sliders and sets each slider to its saved volume. */
	virtual void NativeConstruct() override;

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
