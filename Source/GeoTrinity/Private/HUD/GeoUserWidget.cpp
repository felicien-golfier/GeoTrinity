// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/GeoUserWidget.h"

#include "GeoTrinity/GeoTrinity.h"

void UGeoUserWidget::InitFromHUD(AGeoHUD* GeoHUD)
{
	if (!GeoHUD)
	{
		UE_LOG(LogGeoTrinity, Warning, TEXT("InitFromHUD in widget %s has not been proceeded"), *GetName());
		return;
	}

	BindCallacksFromHUD(GeoHUD);
}