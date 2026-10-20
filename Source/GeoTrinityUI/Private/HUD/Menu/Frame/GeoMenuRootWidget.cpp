// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/Frame/GeoMenuRootWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Engine/GameInstance.h"
#include "HUD/Menu/Frame/GeoMenuPageWidget.h"
#include "HUD/Menu/Frame/GeoMessageSubsystem.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMenuRootWidget::NativeConstruct()
{
	Super::NativeConstruct();

	PageStack.Reset();
	ShowTop();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMenuRootWidget::NativeDestruct()
{
	ClearMessages();

	Super::NativeDestruct();
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoMenuRootWidget::HandleEscapeAction()
{
	CloseMenu();
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
UGeoMenuPageWidget* UGeoMenuRootWidget::OpenPage(TSubclassOf<UGeoMenuPageWidget> const PageClass)
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
		return nullptr;
	}

	PageStack.Add(Page);
	ShowTop();
	FocusTop();
	return Page;
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
	ClearMessages();
	PageStack.Reset();
	ShowTop();
	FocusTop();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMenuRootWidget::ClearMessages() const
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		GameInstance->GetSubsystem<UGeoMessageSubsystem>()->Clear();
	}
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
	TopLevel->SetVisibility(PageStack.IsEmpty() ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
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
