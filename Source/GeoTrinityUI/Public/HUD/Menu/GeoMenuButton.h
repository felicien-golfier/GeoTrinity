// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/Menu/GeoMenuPanelWidget.h"
#include "HUD/Style/GeoUITheme.h"

#include "GeoMenuButton.generated.h"

class UGeoButton;
class UGeoFrame;
class UGeoFrameStyle;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGeoButtonClickedSignature);

/**
 * Reusable button widget. Its look is themed, never authored per button: the label wears TextRole from the UI theme
 * and the optional "Frame" wears its frame style (FrameStyle overrides it for one button), so every button changes
 * together. All click logic is wired through the BlueprintAssignable OnClicked delegate in C++.
 * Required in the BP hierarchy: a UGeoButton named "ButtonWidget". Optional: a UTextBlock named "ButtonText" and a
 * UGeoFrame named "Frame" around the button, which goes active on hover and gamepad focus.
 */
UCLASS()
class GEOTRINITYUI_API UGeoMenuButton : public UGeoMenuPanelWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "GeoButton")
	FGeoButtonClickedSignature OnClicked;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoButton|Appearance")
	FText Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoButton|Appearance")
	EGeoTextRole TextRole = EGeoTextRole::Button;

	/** Frame style for this button only; empty keeps the one the Frame widget wears. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoButton|Appearance")
	TObjectPtr<UGeoFrameStyle> FrameStyle;

	/** Returns the inner UGeoButton that receives forwarded focus and fires the click delegate. */
	UGeoButton* GetButtonWidget() const
	{
		return ButtonWidget;
	}

protected:
	/** Applies visual style from properties for design-time preview in the Blueprint editor. */
	virtual void NativePreConstruct() override;
	/** Binds ButtonWidget's click delegate to HandleButtonClicked. */
	virtual void NativeConstruct() override;
	/** Forwards focus directly to ButtonWidget so the inner UGeoButton enters its hover/focus state. */
	virtual FReply NativeOnFocusReceived(FGeometry const& InGeometry, FFocusEvent const& InFocusEvent) override;
	/** Returns ButtonWidget so gamepad navigation descends into the inner button. */
	virtual UWidget* GetInitialFocusWidget() const override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoButton> ButtonWidget;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ButtonText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoFrame> Frame;

private:
	void ApplyStyle();

	UFUNCTION()
	void HandleButtonClicked();
};
