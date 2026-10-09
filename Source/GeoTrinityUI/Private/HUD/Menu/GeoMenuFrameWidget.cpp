// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoMenuFrameWidget.h"

#include "Components/OverlaySlot.h"
#include "HUD/Style/GeoUITheme.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMenuFrameWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	UOverlaySlot* FrameSlot = Cast<UOverlaySlot>(FrameBox->Slot);
	if (!ensureMsgf(FrameSlot, TEXT("%hs: FrameBox of %s is not in an Overlay"), __FUNCTION__, *GetName()))
	{
		return;
	}

	FrameSlot->SetHorizontalAlignment(HAlign_Fill);
	FrameSlot->SetVerticalAlignment(VAlign_Fill);
	if (UGeoUITheme const* Theme = UGeoUITheme::Get())
	{
		FrameSlot->SetPadding(Theme->MenuFrameMargin);
	}
}
