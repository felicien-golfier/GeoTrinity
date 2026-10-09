// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Style/GeoThemedInputs.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoEditableTextBox::SynchronizeProperties()
{
	if (UGeoUITheme const* Theme = UGeoUITheme::Get())
	{
		FEditableTextBoxStyle Style = Theme->EditableTextBoxStyle;
		if (bOverridePadding)
		{
			Style.SetPadding(PaddingOverride);
		}
		SetWidgetStyle(Style);
	}
	Super::SynchronizeProperties();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoEditableTextBox::SetPaddingOverride(FMargin const& InPadding)
{
	bOverridePadding = true;
	PaddingOverride = InPadding;
	if (MyEditableTextBlock)
	{
		SynchronizeProperties();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoComboBoxString::SynchronizeProperties()
{
	if (UGeoUITheme const* Theme = UGeoUITheme::Get())
	{
		SetWidgetStyle(Theme->ComboBoxStyle);
		SetItemStyle(Theme->ComboBoxItemStyle);
	}
	Super::SynchronizeProperties();
}

// ---------------------------------------------------------------------------------------------------------------------
TSharedRef<SWidget> UGeoComboBoxString::RebuildWidget()
{
	UGeoUITheme const* Theme = UGeoUITheme::Get();
	if (FGeoTextStyle const* TextStyle =
			Theme && TextRole != EGeoTextRole::Custom ? Theme->FindTextStyle(TextRole) : nullptr)
	{
		InitFont(TextStyle->Font);
		InitForegroundColor(TextStyle->Color);
	}
	return Super::RebuildWidget();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCheckBox::SynchronizeProperties()
{
	if (UGeoUITheme const* Theme = UGeoUITheme::Get())
	{
		SetWidgetStyle(Theme->CheckBoxStyle);
	}
	Super::SynchronizeProperties();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSlider::SynchronizeProperties()
{
	if (UGeoUITheme const* Theme = UGeoUITheme::Get())
	{
		SetWidgetStyle(Theme->SliderStyle);
	}
	Super::SynchronizeProperties();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoProgressBar::SynchronizeProperties()
{
	if (UGeoUITheme const* Theme = UGeoUITheme::Get())
	{
		SetWidgetStyle(Theme->ProgressBarStyle);
	}
	Super::SynchronizeProperties();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoScrollBox::SynchronizeProperties()
{
	if (UGeoUITheme const* Theme = UGeoUITheme::Get())
	{
		SetWidgetBarStyle(Theme->ScrollBarStyle);
	}
	Super::SynchronizeProperties();
}

#if WITH_EDITOR
// ---------------------------------------------------------------------------------------------------------------------
FText const UGeoEditableTextBox::GetPaletteCategory()
{
	return INVTEXT("Geo");
}

// ---------------------------------------------------------------------------------------------------------------------
FText const UGeoComboBoxString::GetPaletteCategory()
{
	return INVTEXT("Geo");
}

// ---------------------------------------------------------------------------------------------------------------------
FText const UGeoCheckBox::GetPaletteCategory()
{
	return INVTEXT("Geo");
}

// ---------------------------------------------------------------------------------------------------------------------
FText const UGeoSlider::GetPaletteCategory()
{
	return INVTEXT("Geo");
}

// ---------------------------------------------------------------------------------------------------------------------
FText const UGeoProgressBar::GetPaletteCategory()
{
	return INVTEXT("Geo");
}

// ---------------------------------------------------------------------------------------------------------------------
FText const UGeoScrollBox::GetPaletteCategory()
{
	return INVTEXT("Geo");
}
#endif
