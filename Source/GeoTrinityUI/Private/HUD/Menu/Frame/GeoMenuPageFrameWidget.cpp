// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/Frame/GeoMenuPageFrameWidget.h"

#include "Components/PanelWidget.h"
#include "HUD/Menu/Frame/GeoButton.h"
#include "HUD/Menu/Frame/GeoMenuButton.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMenuPageFrameWidget::HideButtons()
{
	for (UWidget* Corner : {static_cast<UWidget*>(BackButton), static_cast<UWidget*>(CloseButton)})
	{
		while (Corner->GetParent())
		{
			Corner = Corner->GetParent();
		}

		Corner->SetVisibility(ESlateVisibility::Collapsed);
	}
}
