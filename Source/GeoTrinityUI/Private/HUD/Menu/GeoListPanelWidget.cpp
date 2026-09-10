// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoListPanelWidget.h"

#include "HUD/Menu/GeoListRowWidget.h"
#include "HUD/Menu/GeoMenuButton.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoListPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BackButton->OnClicked.AddUniqueDynamic(this, &UGeoListPanelWidget::HandleBack);
}

// ---------------------------------------------------------------------------------------------------------------------
UWidget* UGeoListPanelWidget::GetInitialFocusWidget() const
{
	return BackButton;
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoListPanelWidget::HandleBackAction()
{
	HandleBack();
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
UGeoListRowWidget* UGeoListPanelWidget::MakeRow()
{
	if (!ensureMsgf(RowWidgetClass, TEXT("%hs: RowWidgetClass is not set"), __FUNCTION__))
	{
		return nullptr;
	}

	return CreateWidget<UGeoListRowWidget>(GetOwningPlayer(), RowWidgetClass);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoListPanelWidget::HandleBack()
{
	OnClosed.Broadcast();
}
