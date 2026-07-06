// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "GeoSettingsWidget.generated.h"

class UGeoMenuButton;
class USlider;
class UScrollBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGeoSettingsClosedSignature);

/**
 * Settings panel: sound and key-bindings starting point. MasterVolumeSlider is a placeholder with no audio
 * mixer wired up yet. KeyBindingsList is a read-only list built from UAbilityInfo — no remapping.
 * Communicates back to the parent menu exclusively via the OnClosed delegate.
 * Required in the BP hierarchy: UGeoMenuButton "BackButton", USlider "MasterVolumeSlider", UScrollBox "KeyBindingsList".
 */
UCLASS()
class GEOTRINITYUI_API UGeoSettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Menu")
	FGeoSettingsClosedSignature OnClosed;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> BackButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<USlider> MasterVolumeSlider;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UScrollBox> KeyBindingsList;

private:
	UFUNCTION()
	void HandleBack();

	UFUNCTION()
	void HandleMasterVolumeChanged(float Value);

	void BuildKeyBindingsList();
};
