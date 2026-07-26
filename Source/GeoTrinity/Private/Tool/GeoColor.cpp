// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Tool/GeoColor.h"

#include "Settings/GameDataSettings.h"

// ---------------------------------------------------------------------------------------------------------------------
FLinearColor FGeoColorParam::GetColor() const
{
	if (Color == EGeoColor::Override)
	{
		return OverrideColor;
	}

	FLinearColor const* const PaletteColor = GetDefault<UGameDataSettings>()->ColorPalette.Find(Color);
	if (!ensureMsgf(PaletteColor, TEXT("FGeoColorParam: %s has no entry in the Game Data Settings color palette"),
					*UEnum::GetValueAsString(Color)))
	{
		return FLinearColor::White;
	}

	return *PaletteColor;
}
