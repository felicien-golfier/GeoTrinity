// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/List/GeoListPanelWidget.h"

#include "HUD/Menu/List/GeoListRowWidget.h"

// ---------------------------------------------------------------------------------------------------------------------
UGeoListRowWidget* UGeoListPanelWidget::MakeRow()
{
	if (!ensureMsgf(RowWidgetClass, TEXT("%hs: RowWidgetClass is not set"), __FUNCTION__))
	{
		return nullptr;
	}

	return CreateWidget<UGeoListRowWidget>(GetOwningPlayer(), RowWidgetClass);
}
