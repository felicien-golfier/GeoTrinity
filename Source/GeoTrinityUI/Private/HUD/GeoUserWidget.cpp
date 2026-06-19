// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/GeoUserWidget.h"

#include "GeoTrinity/GeoTrinity.h"
#include "HUD/HudFunctionLibrary.h"

void UGeoUserWidget::InitFromHUD(AGeoHUD* GeoHUD)
{
	if (!GeoHUD)
	{
		UE_LOG(LogGeoTrinity, Warning, TEXT("InitFromHUD in widget %s has not been proceeded"), *GetName());
		return;
	}

	BindCallbacksFromHUD(GeoHUD);
}
