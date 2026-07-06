// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoSettingsWidget.h"

#include "AbilitySystem/Data/AbilityInfo.h"
#include "Components/ScrollBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "HUD/Menu/GeoMenuButton.h"
#include "InputAction.h"
#include "Settings/GameDataSettings.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!BackButton)
	{
		ensureMsgf(BackButton, TEXT("UGeoSettingsWidget: BackButton is not bound"));
		return;
	}
	if (!MasterVolumeSlider)
	{
		ensureMsgf(MasterVolumeSlider, TEXT("UGeoSettingsWidget: MasterVolumeSlider is not bound"));
		return;
	}
	if (!KeyBindingsList)
	{
		ensureMsgf(KeyBindingsList, TEXT("UGeoSettingsWidget: KeyBindingsList is not bound"));
		return;
	}

	BackButton->OnClicked.AddDynamic(this, &UGeoSettingsWidget::HandleBack);
	MasterVolumeSlider->OnValueChanged.AddDynamic(this, &UGeoSettingsWidget::HandleMasterVolumeChanged);

	KeyBindingsList->ClearChildren();
	BuildKeyBindingsList();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSettingsWidget::NativeDestruct()
{
	if (BackButton)
	{
		BackButton->OnClicked.RemoveDynamic(this, &UGeoSettingsWidget::HandleBack);
	}
	if (MasterVolumeSlider)
	{
		MasterVolumeSlider->OnValueChanged.RemoveDynamic(this, &UGeoSettingsWidget::HandleMasterVolumeChanged);
	}
	Super::NativeDestruct();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSettingsWidget::HandleBack()
{
	OnClosed.Broadcast();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSettingsWidget::HandleMasterVolumeChanged(float /*Value*/)
{
	// Placeholder: no sound-class/mixer convention exists in the project yet.
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSettingsWidget::BuildKeyBindingsList()
{
	UAbilityInfo* AbilityInfo = UGameDataSettings::GetLoadedDataAsset(GetDefault<UGameDataSettings>()->AbilityInfo);
	if (!ensureMsgf(AbilityInfo, TEXT("UGeoSettingsWidget: AbilityInfo is not set in GameDataSettings")))
	{
		return;
	}

	for (FPlayersGameplayAbilityInfo const& Info : AbilityInfo->GetAllPlayersAbilityInfos())
	{
		if (!Info.InputAction)
		{
			continue;
		}

		UTextBlock* Row = NewObject<UTextBlock>(this);
		Row->SetText(FText::FromString(
			FString::Printf(TEXT("%s: %s"), *Info.InputTag.ToString(), *Info.InputAction->GetName())));
		KeyBindingsList->AddChild(Row);
	}
}
