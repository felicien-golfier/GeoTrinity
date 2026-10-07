// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Style/GeoText.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoText::SynchronizeProperties()
{
	UGeoUITheme::ApplyTextStyle(this, Role);
	Super::SynchronizeProperties();
}

#if WITH_EDITOR
// ---------------------------------------------------------------------------------------------------------------------
FText const UGeoText::GetPaletteCategory()
{
	return INVTEXT("Geo");
}
#endif
