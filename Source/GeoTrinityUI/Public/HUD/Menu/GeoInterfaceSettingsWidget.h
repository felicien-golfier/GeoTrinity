// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/Menu/GeoMenuPageWidget.h"

#include "GeoInterfaceSettingsWidget.generated.h"

class UCheckBox;

/**
 * Interface settings panel: what the HUD shows, each option read from and written straight to UGeoGameUserSettings.
 * Required in the BP hierarchy: UCheckBox "CombatStatsCheckBox".
 */
UCLASS()
class GEOTRINITYUI_API UGeoInterfaceSettingsWidget : public UGeoMenuPageWidget
{
	GENERATED_BODY()

protected:
	/** Wires the checkbox and sets the checkbox to its saved value. */
	virtual void NativeConstruct() override;
	/** Returns CombatStatsCheckBox. */
	virtual UWidget* GetInitialFocusWidget() const override;

	/** Shows the combat stats panel on the HUD. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCheckBox> CombatStatsCheckBox;

private:
	UFUNCTION()
	void HandleCombatStatsChanged(bool bIsChecked);
};
