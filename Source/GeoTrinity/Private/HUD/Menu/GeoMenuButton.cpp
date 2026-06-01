// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoMenuButton.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Styling/SlateTypes.h"

void UGeoMenuButton::NativePreConstruct()
{
	Super::NativePreConstruct();
	ApplyStyle();
}

void UGeoMenuButton::NativeConstruct()
{
	Super::NativeConstruct();
	ApplyStyle();

	if (!ButtonWidget)
	{
		ensureMsgf(ButtonWidget, TEXT("UGeoMenuButton: Button widget is not bound — add a UButton named 'Button' to the widget hierarchy"));
		return;
	}

	ButtonWidget->OnClicked.AddDynamic(this, &UGeoMenuButton::HandleButtonClicked);
}

void UGeoMenuButton::ApplyStyle()
{
	if (ButtonText)
	{
		ButtonText->SetText(Label);
		ButtonText->SetFont(Font);
		ButtonText->SetColorAndOpacity(TextColor);
	}

	if (ButtonWidget)
	{
		FButtonStyle Style = ButtonWidget->GetStyle();
		Style.Normal = NormalBrush;
		Style.Hovered = HoveredBrush;
		Style.Pressed = PressedBrush;
		ButtonWidget->SetStyle(Style);
	}
}

void UGeoMenuButton::HandleButtonClicked()
{
	OnClicked.Broadcast();
}
