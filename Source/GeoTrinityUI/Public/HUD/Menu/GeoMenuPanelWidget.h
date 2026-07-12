// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "GeoMenuPanelWidget.generated.h"

/**
 * Base for gamepad-navigable menu widgets. The panel is focusable and holds focus itself while nothing is
 * selected: the first navigation input (d-pad/stick) moves focus to GetInitialFocusWidget(), and any mouse
 * move while a menu button is selected returns focus to the panel so the gamepad highlight clears. Gamepad
 * B / BackSpace bubble up from the focused child and trigger HandleBackAction() on the innermost panel that
 * overrides it.
 */
UCLASS(Abstract)
class GEOTRINITYUI_API UGeoMenuPanelWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	/** Marks the panel focusable so it can hold focus between selections. */
	virtual void NativeConstruct() override;
	/** Routes gamepad B / BackSpace to HandleBackAction; on any navigation input while the panel itself is focused, advances focus to GetInitialFocusWidget(). */
	virtual FReply NativeOnKeyDown(FGeometry const& InGeometry, FKeyEvent const& InKeyEvent) override;
	/** Advances focus to GetInitialFocusWidget() on the first d-pad/stick input while the panel itself is focused. */
	virtual FReply NativeOnAnalogValueChanged(FGeometry const& InGeometry,
											  FAnalogInputEvent const& InAnalogEvent) override;
	/** Returns focus to the panel when the mouse moves while an SGeoButton is focused, clearing the gamepad highlight. */
	virtual FReply NativeOnMouseMove(FGeometry const& InGeometry, FPointerEvent const& InMouseEvent) override;

	/** Widget that receives focus on the first navigation input while the panel itself is focused. */
	virtual UWidget* GetInitialFocusWidget() const
		PURE_VIRTUAL(UGeoMenuPanelWidget::GetInitialFocusWidget, return nullptr;);

	/** Back/cancel input (gamepad B, BackSpace). Return true if consumed; default leaves the key unhandled. */
	virtual bool HandleBackAction()
	{
		return false;
	}

private:
	// Screen-space position at the last mouse-move that actually stole focus. Distinguishes real mouse
	// motion from OS-level jitter, which otherwise fires NativeOnMouseMove every frame and keeps yanking
	// focus back from gamepad navigation.
	FVector2D LastFocusStealMousePosition = FVector2D::ZeroVector;
	bool bHasLastFocusStealMousePosition = false;
};
