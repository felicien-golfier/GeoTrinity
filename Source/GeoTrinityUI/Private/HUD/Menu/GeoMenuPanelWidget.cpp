// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoMenuPanelWidget.h"

#include "Components/Widget.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMenuPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	bHasLastFocusStealMousePosition = false;
}

// ---------------------------------------------------------------------------------------------------------------------
FReply UGeoMenuPanelWidget::NativeOnKeyDown(FGeometry const& InGeometry, FKeyEvent const& InKeyEvent)
{
	if ((InKeyEvent.GetKey() == EKeys::Gamepad_FaceButton_Right || InKeyEvent.GetKey() == EKeys::BackSpace)
		&& HandleBackAction())
	{
		return FReply::Handled();
	}

	// Only when the panel itself is focused (nothing selected): key events bubble up the focus path, so
	// without this guard a navigation key travelling from a focused button would be swallowed here and
	// break Slate's button-to-button navigation.
	UWidget* InitialFocusWidget = GetInitialFocusWidget();
	if (InitialFocusWidget && HasUserFocus(GetOwningPlayer())
		&& FSlateApplication::Get().GetNavigationDirectionFromKey(InKeyEvent) != EUINavigation::Invalid)
	{
		if (APlayerController* OwningPlayer = GetOwningPlayer())
		{
			OwningPlayer->CurrentMouseCursor = EMouseCursor::None;
			OwningPlayer->SetShowMouseCursor(false);
		}
		return FReply::Handled().SetUserFocus(InitialFocusWidget->TakeWidget(), EFocusCause::Navigation);
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

// ---------------------------------------------------------------------------------------------------------------------
FReply UGeoMenuPanelWidget::NativeOnAnalogValueChanged(FGeometry const& InGeometry,
													   FAnalogInputEvent const& InAnalogEvent)
{
	UWidget* InitialFocusWidget = GetInitialFocusWidget();
	if (InitialFocusWidget && HasUserFocus(GetOwningPlayer())
		&& FSlateApplication::Get().GetNavigationDirectionFromAnalog(InAnalogEvent) != EUINavigation::Invalid)
	{
		if (APlayerController* OwningPlayer = GetOwningPlayer())
		{
			OwningPlayer->CurrentMouseCursor = EMouseCursor::None;
			OwningPlayer->SetShowMouseCursor(false);
		}
		return FReply::Handled().SetUserFocus(InitialFocusWidget->TakeWidget(), EFocusCause::Navigation);
	}
	return Super::NativeOnAnalogValueChanged(InGeometry, InAnalogEvent);
}

// ---------------------------------------------------------------------------------------------------------------------
FReply UGeoMenuPanelWidget::NativeOnMouseMove(FGeometry const& InGeometry, FPointerEvent const& InMouseEvent)
{
	// Mouse takes over: pull focus off any gamepad-selected menu button ("SGeoButton" is the Slate type
	// UGeoButton builds) so only the widget really under the cursor shows as hovered. On a UGeoMenuButton
	// this refocuses its own inner button via NativeOnFocusReceived, keeping selection and hover in sync.
	FVector2D const MousePosition = InMouseEvent.GetScreenSpacePosition();
	bool const bMoved = !bHasLastFocusStealMousePosition || !MousePosition.Equals(LastFocusStealMousePosition, 1.f);
	LastFocusStealMousePosition = MousePosition;
	bHasLastFocusStealMousePosition = true;

	if (bMoved)
	{
		if (APlayerController* OwningPlayer = GetOwningPlayer())
		{
			OwningPlayer->CurrentMouseCursor = EMouseCursor::Crosshairs;
			OwningPlayer->SetShowMouseCursor(true);
		}
	}

	TSharedPtr<SWidget> FocusedWidget = FSlateApplication::Get().GetUserFocusedWidget(InMouseEvent.GetUserIndex());
	if (bMoved && FocusedWidget && FocusedWidget->GetType() == "SGeoButton")
	{
		return FReply::Handled().SetUserFocus(TakeWidget(), EFocusCause::Mouse);
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}
