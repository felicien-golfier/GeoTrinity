// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateColor.h"

#include "GeoMenuButton.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGeoButtonClickedSignature);

/**
 * Reusable styled button widget. Blueprint subclasses configure appearance via EditAnywhere properties;
 * all click logic is wired through the BlueprintAssignable OnClicked delegate in C++.
 * Required in the BP hierarchy: a UButton named "Button". Optional: a UTextBlock named "ButtonText".
 */
UCLASS()
class GEOTRINITYUI_API UGeoMenuButton : public UUserWidget
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

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> ButtonWidget;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ButtonText;

private:
	void ApplyStyle();

	UFUNCTION()
	void HandleButtonClicked();
};
