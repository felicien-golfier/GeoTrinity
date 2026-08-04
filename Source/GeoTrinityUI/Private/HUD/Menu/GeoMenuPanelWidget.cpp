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

	if (FSlateApplication::Get().GetNavigationDirectionFromKey(InKeyEvent) != EUINavigation::Invalid)
	{
		FReply const Reply = FocusInitialWidgetForNavigation();
		if (Reply.IsEventHandled())
		{
			return Reply;
		}
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

// ---------------------------------------------------------------------------------------------------------------------
FReply UGeoMenuPanelWidget::NativeOnAnalogValueChanged(FGeometry const& InGeometry,
													   FAnalogInputEvent const& InAnalogEvent)
{
	if (FSlateApplication::Get().GetNavigationDirectionFromAnalog(InAnalogEvent) != EUINavigation::Invalid)
	{
		FReply const Reply = FocusInitialWidgetForNavigation();
		if (Reply.IsEventHandled())
		{
			return Reply;
		}
	}
	return Super::NativeOnAnalogValueChanged(InGeometry, InAnalogEvent);
}

// ---------------------------------------------------------------------------------------------------------------------
FReply UGeoMenuPanelWidget::NativeOnMouseMove(FGeometry const& InGeometry, FPointerEvent const& InMouseEvent)
{
	FVector2D const MousePosition = InMouseEvent.GetScreenSpacePosition();
	bool const bMoved = !bHasLastFocusStealMousePosition || !MousePosition.Equals(LastFocusStealMousePosition, 1.f);
	LastFocusStealMousePosition = MousePosition;
	bHasLastFocusStealMousePosition = true;
	if (!bMoved)
	{
		return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
	}

	SetMouseCursorVisible(true);

	// Mouse takes over: pull focus off any gamepad-selected menu button ("SGeoButton" is the Slate type
	// UGeoButton builds) so only the widget really under the cursor shows as hovered. On a UGeoMenuButton
	// this refocuses its own inner button via NativeOnFocusReceived, keeping selection and hover in sync.
	TSharedPtr<SWidget> const FocusedWidget =
		FSlateApplication::Get().GetUserFocusedWidget(InMouseEvent.GetUserIndex());
	if (FocusedWidget && FocusedWidget->GetType() == "SGeoButton")
	{
		return FReply::Handled().SetUserFocus(TakeWidget(), EFocusCause::Mouse);
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

// ---------------------------------------------------------------------------------------------------------------------
FReply UGeoMenuPanelWidget::FocusInitialWidgetForNavigation()
{
	UWidget* InitialFocusWidget = GetInitialFocusWidget();
	if (!InitialFocusWidget || !HasUserFocus(GetOwningPlayer()))
	{
		return FReply::Unhandled();
	}

	SetMouseCursorVisible(false);
	return FReply::Handled().SetUserFocus(InitialFocusWidget->TakeWidget(), EFocusCause::Navigation);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMenuPanelWidget::SetMouseCursorVisible(bool const bVisible)
{
	APlayerController* OwningPlayer = GetOwningPlayer();
	if (!OwningPlayer)
	{
		return;
	}

	OwningPlayer->CurrentMouseCursor = bVisible ? EMouseCursor::Crosshairs : EMouseCursor::None;
	OwningPlayer->SetShowMouseCursor(bVisible);
}
