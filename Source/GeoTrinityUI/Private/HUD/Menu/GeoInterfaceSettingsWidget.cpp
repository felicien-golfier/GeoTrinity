// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoInterfaceSettingsWidget.h"

#include "Components/CheckBox.h"
#include "HUD/Menu/GeoMenuButton.h"
#include "Settings/GeoGameUserSettings.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoInterfaceSettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CombatStatsCheckBox->SetIsChecked(UGeoGameUserSettings::Get()->ShowCombatStats());

	CombatStatsCheckBox->OnCheckStateChanged.AddUniqueDynamic(this,
															  &UGeoInterfaceSettingsWidget::HandleCombatStatsChanged);
}

// ---------------------------------------------------------------------------------------------------------------------
UWidget* UGeoInterfaceSettingsWidget::GetInitialFocusWidget() const
{
	return CombatStatsCheckBox;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoInterfaceSettingsWidget::HandleCombatStatsChanged(bool const bIsChecked)
{
	UGeoGameUserSettings::Get()->SetShowCombatStats(bIsChecked);
}
