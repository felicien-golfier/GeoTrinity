// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/Settings/GeoInterfaceSettingsWidget.h"

#include "Components/CheckBox.h"
#include "HUD/Menu/Frame/GeoMenuButton.h"
#include "Settings/GeoGameUserSettings.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoInterfaceSettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CombatStatsCheckBox->SetIsChecked(UGeoGameUserSettings::Get()->ShowCombatStats());

	CombatStatsCheckBox->OnCheckStateChanged.AddUniqueDynamic(this,
															  &UGeoInterfaceSettingsWidget::HandleCombatStatsChanged);

	if (CatLineAutoSkipCheckBox)
	{
		CatLineAutoSkipCheckBox->SetIsChecked(UGeoGameUserSettings::Get()->CatLineAutoSkip());
		CatLineAutoSkipCheckBox->OnCheckStateChanged.AddUniqueDynamic(
			this, &UGeoInterfaceSettingsWidget::HandleCatLineAutoSkipChanged);
	}
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

// ---------------------------------------------------------------------------------------------------------------------
void UGeoInterfaceSettingsWidget::HandleCatLineAutoSkipChanged(bool const bIsChecked)
{
	UGeoGameUserSettings::Get()->SetCatLineAutoSkip(bIsChecked);
}
