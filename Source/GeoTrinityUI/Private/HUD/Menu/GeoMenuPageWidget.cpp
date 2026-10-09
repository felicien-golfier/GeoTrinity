// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoMenuPageWidget.h"

#include "HUD/Menu/GeoButton.h"
#include "HUD/Menu/GeoMenuButton.h"
#include "HUD/Menu/GeoMenuPageFrameWidget.h"
#include "HUD/Menu/GeoMenuRootWidget.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMenuPageWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (PageFrame && IsInMenu())
	{
		PageFrame->GetBackButton()->OnClicked.AddUniqueDynamic(this, &UGeoMenuPageWidget::GoBack);
		PageFrame->GetCloseButton()->OnClicked.AddUniqueDynamic(this, &UGeoMenuPageWidget::CloseMenu);
	}
	else if (PageFrame)
	{
		PageFrame->HideButtons();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
UWidget* UGeoMenuPageWidget::GetInitialFocusWidget() const
{
	return GetBackButton();
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoMenuPageWidget::HandleBackAction()
{
	GoBack();
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMenuPageWidget::OpenPage(TSubclassOf<UGeoMenuPageWidget> const PageClass) const
{
	if (UGeoMenuRootWidget* Menu = GetMenu())
	{
		Menu->OpenPage(PageClass);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoMenuPageWidget::IsInMenu() const
{
	return GetTypedOuter<UGeoMenuRootWidget>() != nullptr;
}

// ---------------------------------------------------------------------------------------------------------------------
UGeoMenuButton* UGeoMenuPageWidget::GetBackButton() const
{
	return PageFrame ? PageFrame->GetBackButton() : nullptr;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMenuPageWidget::GoBack()
{
	if (UGeoMenuRootWidget* Menu = GetMenu())
	{
		Menu->GoBack();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMenuPageWidget::CloseMenu()
{
	if (UGeoMenuRootWidget* Menu = GetMenu())
	{
		Menu->CloseMenu();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
UGeoMenuRootWidget* UGeoMenuPageWidget::GetMenu() const
{
	UGeoMenuRootWidget* Menu = GetTypedOuter<UGeoMenuRootWidget>();
	ensureMsgf(Menu, TEXT("%hs: page %s is not in a UGeoMenuRootWidget's tree"), __FUNCTION__, *GetName());
	return Menu;
}
