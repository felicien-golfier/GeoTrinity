// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoMenuButton.h"

#include "Components/TextBlock.h"
#include "HUD/Menu/GeoButton.h"
#include "HUD/Style/GeoFrame.h"

void UGeoMenuButton::NativePreConstruct()
{
	Super::NativePreConstruct();
	ApplyStyle();
}

void UGeoMenuButton::NativeConstruct()
{
	Super::NativeConstruct();

	ensureMsgf(ButtonWidget->GetIsFocusable(),
			   TEXT("UGeoMenuButton: ButtonWidget is not focusable — forwarded focus lands back on this widget"));

	ButtonWidget->OnClicked.AddUniqueDynamic(this, &UGeoMenuButton::HandleButtonClicked);
}

FReply UGeoMenuButton::NativeOnFocusReceived(FGeometry const& InGeometry, FFocusEvent const& InFocusEvent)
{
	return FReply::Handled().SetUserFocus(ButtonWidget->TakeWidget(), InFocusEvent.GetCause());
}

UWidget* UGeoMenuButton::GetInitialFocusWidget() const
{
	return ButtonWidget;
}

void UGeoMenuButton::SetLabel(FText const& InLabel)
{
	Label = InLabel;
	ApplyStyle();
}

void UGeoMenuButton::ApplyStyle()
{
	if (ButtonText)
	{
		ButtonText->SetText(Label);
		UGeoUITheme::ApplyTextStyle(ButtonText, TextRole);
	}

	if (Frame && FrameStyle)
	{
		Frame->SetFrameStyle(FrameStyle);
	}
}

void UGeoMenuButton::HandleButtonClicked()
{
	OnClicked.Broadcast();
}
