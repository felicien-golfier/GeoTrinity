// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/Menu/GeoMenuPanelWidget.h"

#include "GeoSoundSettingsWidget.generated.h"

class UGeoMenuButton;
class USlider;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGeoSoundSettingsClosedSignature);

/**
 * Sound settings panel. MasterVolumeSlider is a placeholder with no audio mixer wired up yet.
 * Communicates back to the parent menu exclusively via the OnClosed delegate.
 * Required in the BP hierarchy: UGeoMenuButton "BackButton", USlider "MasterVolumeSlider".
 */
UCLASS()
class GEOTRINITYUI_API UGeoSoundSettingsWidget : public UGeoMenuPanelWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Menu")
	FGeoSoundSettingsClosedSignature OnClosed;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual UWidget* GetInitialFocusWidget() const override;
	virtual bool HandleBackAction() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> BackButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<USlider> MasterVolumeSlider;

private:
	UFUNCTION()
	void HandleBack();

	UFUNCTION()
	void HandleMasterVolumeChanged(float Value);
};
