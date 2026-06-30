// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/GeoDamageNumberWidget.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoDamageNumberWidget::Activate(float Amount, bool bIsHeal, FVector2D ScreenPos)
{
	bAvailable = false;
	SetRenderTranslation(ScreenPos);
	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetData(Amount, bIsHeal);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoDamageNumberWidget::ReturnToPool()
{
	bAvailable = true;
	SetVisibility(ESlateVisibility::Collapsed);
}
