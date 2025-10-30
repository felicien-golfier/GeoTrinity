﻿// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/GeoHUD.h"

#include "GeoPlayerController.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystem/GeoAttributeSetBase.h"
#include "Blueprint/UserWidget.h"
#include "HUD/GeoUserWidget.h"

// ---------------------------------------------------------------------------------------------------------------------
// FHudPlayerParams
// ---------------------------------------------------------------------------------------------------------------------
AGeoPlayerController* FHudPlayerParams::GetGeoPlayerController() const
{
	return Cast<AGeoPlayerController>(PlayerController);
}

UGeoAbilitySystemComponent* FHudPlayerParams::GetGeoAbilitySystemComponent() const
{
	return Cast<UGeoAbilitySystemComponent>(AbilitySystemComponent);
}

UGeoAttributeSetBase* FHudPlayerParams::GetGeoAttributeSet() const
{
	return Cast<UGeoAttributeSetBase>(AttributeSet);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoHUD::InitOverlay(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, UAttributeSet* AS)
{
	checkf(OverlayWidgetClass, TEXT("Overlay Widget Class uninitialized, please fill out HUD %s"), *GetName())

	//UUserWidget* Widget = CreateWidget<UUserWidget>(GetWorld(), OverlayWidgetClass);
	//OverlayWidget = Cast<UGeoUserWidget>(Widget);
	//checkf(OverlayWidget, TEXT("OverlayWidgetClass is not of class UGeoUserWidget. Rethink design if this is necessary."))

	OverlayWidget = CreateWidget<UGeoUserWidget>(GetWorld(), OverlayWidgetClass);
	
	// Setup params the HUD may very probably need to access
	HudPlayerParams.PlayerController = PC;
	HudPlayerParams.PlayerState = PS;
	HudPlayerParams.AbilitySystemComponent = ASC;
	HudPlayerParams.AttributeSet = AS;

	OverlayWidget->InitFromHUD(this);
	BroadcastInitialValues();
	BindCallbacksToDependencies();

	OverlayWidget->AddToViewport();
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoHUD::BroadcastInitialValues() const
{
	UGeoAttributeSetBase const* GeoAttributeSet = HudPlayerParams.GetGeoAttributeSet();
	if (!GeoAttributeSet)
		return;

	OnHealthChanged.Broadcast(GeoAttributeSet->GetHealth());
	OnMaxHealthChanged.Broadcast(GeoAttributeSet->GetMaxHealth());
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoHUD::BindCallbacksToDependencies()
{
	UGeoAttributeSetBase const* GeoAttributeSet = HudPlayerParams.GetGeoAttributeSet();
	if (!GeoAttributeSet)
		return;
	
	HudPlayerParams.AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(GeoAttributeSet->GetHealthAttribute()).
		AddWeakLambda(this, [this](const FOnAttributeChangeData& data)
		{
			OnHealthChanged.Broadcast(data.NewValue);
		});
	
	HudPlayerParams.AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(GeoAttributeSet->GetMaxHealthAttribute()).
		AddWeakLambda(this, [this](const FOnAttributeChangeData& data)
		{
			OnMaxHealthChanged.Broadcast(data.NewValue);
		});
}