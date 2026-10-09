// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoMenuRootWidget.h"

#include "Blueprint/WidgetTree.h"
#include "HUD/Menu/GeoMenuPageWidget.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMenuRootWidget::NativeConstruct()
{
	Super::NativeConstruct();

	PageStack.Reset();
	ShowTop();
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoMenuRootWidget::HandleEscapeAction()
{
	CloseMenu();
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMenuRootWidget::OpenPage(TSubclassOf<UGeoMenuPageWidget> const PageClass)
{
	UGeoMenuPageWidget* Page = nullptr;
	WidgetTree->ForEachWidget(
		[&Page, PageClass](UWidget* Widget)
		{
			if (!Page && Widget->IsA(PageClass))
			{
				Page = Cast<UGeoMenuPageWidget>(Widget);
			}
		});
	if (!ensureMsgf(Page, TEXT("%hs: %s holds no %s page"), __FUNCTION__, *GetName(), *GetNameSafe(PageClass)))
	{
		return;
	}

	PageStack.Add(Page);
	ShowTop();
	FocusTop();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMenuRootWidget::GoBack()
{
	if (!ensureMsgf(!PageStack.IsEmpty(), TEXT("%hs: no page open on %s"), __FUNCTION__, *GetName()))
	{
		return;
	}

	PageStack.Pop();
	ShowTop();
	FocusTop();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMenuRootWidget::CloseMenu()
{
	PageStack.Reset();
	ShowTop();
	FocusTop();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMenuRootWidget::ShowTop()
{
	WidgetTree->ForEachWidget(
		[](UWidget* Widget)
		{
			if (UGeoMenuPageWidget* Page = Cast<UGeoMenuPageWidget>(Widget))
			{
				Page->SetVisibility(ESlateVisibility::Collapsed);
			}
		});
	TopLevel->SetVisibility(PageStack.IsEmpty() ? ESlateVisibility::SelfHitTestInvisible
												: ESlateVisibility::Collapsed);
	if (!PageStack.IsEmpty())
	{
		PageStack.Last()->SetVisibility(ESlateVisibility::Visible);
		PageStack.Last()->OnPageShown();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMenuRootWidget::FocusTop()
{
	UWidget* const Top = PageStack.IsEmpty() ? GetInitialFocusWidget() : PageStack.Last().Get();
	Top->SetFocus();
}
