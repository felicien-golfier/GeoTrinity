// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Tool/GeoColor.h"

#include "Settings/GameDataSettings.h"

// ---------------------------------------------------------------------------------------------------------------------
FLinearColor FGeoColorParam::GetColor(float const Alpha) const
{
	FLinearColor ReturnColor = FLinearColor::White;
	if (Color == EGeoColor::Override)
	{
		ReturnColor = OverrideColor;
	}
	else
	{
		FLinearColor const* const PaletteColor = GetDefault<UGameDataSettings>()->ColorPalette.Find(Color);
		if (ensureMsgf(PaletteColor, TEXT("FGeoColorParam: %s has no entry in the Game Data Settings color palette"),
					   *UEnum::GetValueAsString(Color)))
		{
			ReturnColor = *PaletteColor;
		}
	}

	if (Alpha >= 0)
	{
		ReturnColor.A = Alpha;
	}

	return ReturnColor;
}
