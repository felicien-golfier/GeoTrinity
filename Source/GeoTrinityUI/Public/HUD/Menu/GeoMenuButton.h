// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "HUD/Menu/GeoMenuPanelWidget.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateColor.h"

#include "GeoMenuButton.generated.h"

class UGeoButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGeoButtonClickedSignature);

/**
 * Reusable styled button widget. Blueprint subclasses configure appearance via EditAnywhere properties;
 * all click logic is wired through the BlueprintAssignable OnClicked delegate in C++.
 * Required in the BP hierarchy: a UGeoButton named "ButtonWidget". Optional: a UTextBlock named "ButtonText".
 */
UCLASS()
class GEOTRINITYUI_API UGeoMenuButton : public UGeoMenuPanelWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Button")
	FGeoButtonClickedSignature OnClicked;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button|Appearance")
	FText Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button|Appearance")
	FSlateFontInfo Font;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button|Appearance")
	FSlateColor TextColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button|Appearance")
	FSlateBrush NormalBrush;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button|Appearance")
	FSlateBrush HoveredBrush;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button|Appearance")
	FSlateBrush PressedBrush;

	UGeoButton* GetButtonWidget() const
	{
		return ButtonWidget;
	}

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnFocusReceived(FGeometry const& InGeometry, FFocusEvent const& InFocusEvent) override;
	virtual UWidget* GetInitialFocusWidget() const override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoButton> ButtonWidget;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ButtonText;

private:
	void ApplyStyle();

	UFUNCTION()
	void HandleButtonClicked();
};
