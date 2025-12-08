// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/GeoUserWidget.h"

#include "GeoTrinity/GeoTrinity.h"
#include "HUD/HudFunctionLibrary.h"

void UGeoUserWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	if (UHudFunctionLibrary::ShouldDrawHUD(GetOwningPlayer()))
	{
		UE_LOG(LogTemp, Log, TEXT("Removing %s from parent because HUD should not be drawn"), *GetName());
		RemoveFromParent();
	}
}

void UGeoUserWidget::InitFromHUD(AGeoHUD* GeoHUD)
{
	if (!GeoHUD)
	{
		UE_LOG(LogGeoTrinity, Warning, TEXT("InitFromHUD in widget %s has not been proceeded"), *GetName());
		return;
	}

	BindCallbacksFromHUD(GeoHUD);
}