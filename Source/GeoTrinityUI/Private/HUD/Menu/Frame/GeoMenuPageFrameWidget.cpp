// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/Frame/GeoMenuPageFrameWidget.h"

#include "HUD/Menu/Frame/GeoMenuButton.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMenuPageFrameWidget::HideButtons()
{
	BackButton->SetVisibility(ESlateVisibility::Collapsed);
	CloseBox->SetVisibility(ESlateVisibility::Collapsed);
}
