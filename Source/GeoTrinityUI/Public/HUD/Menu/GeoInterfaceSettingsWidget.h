// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/Menu/GeoMenuPanelWidget.h"

#include "GeoInterfaceSettingsWidget.generated.h"

class UCheckBox;
class UGeoMenuButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGeoInterfaceSettingsClosedSignature);

/**
 * Interface settings panel: what the HUD shows, each option read from and written straight to UGeoGameUserSettings.
 * Communicates back to the parent menu exclusively via the OnClosed delegate.
 * Required in the BP hierarchy: UGeoMenuButton "BackButton", UCheckBox "CombatStatsCheckBox".
 */
UCLASS()
class GEOTRINITYUI_API UGeoInterfaceSettingsWidget : public UGeoMenuPanelWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "GeoMenu")
	FGeoInterfaceSettingsClosedSignature OnClosed;

protected:
	/** Wires BackButton and the checkbox, and sets the checkbox to its saved value. */
	virtual void NativeConstruct() override;
	/** Returns CombatStatsCheckBox. */
	virtual UWidget* GetInitialFocusWidget() const override;
	/** Fires OnClosed and consumes the back input. */
	virtual bool HandleBackAction() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> BackButton;

	/** Shows the combat stats panel on the HUD. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCheckBox> CombatStatsCheckBox;

private:
	UFUNCTION()
	void HandleBack();

	UFUNCTION()
	void HandleCombatStatsChanged(bool bIsChecked);
};
