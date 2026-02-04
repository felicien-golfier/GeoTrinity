// Fill out your copyright notice in the Description page of Project Settings.

#include "HUD/GeoHUD.h"

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Blueprint/UserWidget.h"
#include "Characters/EnemyCharacter.h"
#include "GeoGameState.h"
#include "GeoPlayerController.h"
#include "HUD/GenericCombattantWidget.h"
#include "HUD/GeoUserWidget.h"
#include "HUD/HudFunctionLibrary.h"

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

		// UUserWidget* Widget = CreateWidget<UUserWidget>(GetWorld(), OverlayWidgetClass);
		// OverlayWidget = Cast<UGeoUserWidget>(Widget);
		// checkf(OverlayWidget, TEXT("OverlayWidgetClass is not of class UGeoUserWidget. Rethink design if this is
		// necessary."))

		// Setup params the HUD may very probably need to access
		HudPlayerParams.PlayerController = PC;
	HudPlayerParams.PlayerState = PS;
	HudPlayerParams.AbilitySystemComponent = ASC;
	HudPlayerParams.AttributeSet = AS;

	if (UHudFunctionLibrary::ShouldDrawHUD(GetOwner()))
	{
		OverlayWidget = CreateWidget<UGeoUserWidget>(GetWorld(), OverlayWidgetClass);
		OverlayWidget->InitFromHUD(this);
		BroadcastInitialValues();
		BindCallbacksToDependencies();

		OverlayWidget->AddToViewport();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoHUD::BroadcastInitialValues() const
{
	UGeoAttributeSetBase const* GeoAttributeSet = HudPlayerParams.GetGeoAttributeSet();
	if (!GeoAttributeSet)
	{
		return;
	}

	OnHealthChanged.Broadcast(GeoAttributeSet->GetHealth());
	OnMaxHealthChanged.Broadcast(GeoAttributeSet->GetMaxHealth());
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoHUD::BindCallbacksToDependencies()
{
	UGeoAttributeSetBase const* GeoAttributeSet = HudPlayerParams.GetGeoAttributeSet();
	if (!GeoAttributeSet)
	{
		return;
	}

	HudPlayerParams.AbilitySystemComponent
		->GetGameplayAttributeValueChangeDelegate(GeoAttributeSet->GetHealthAttribute())
		.AddWeakLambda(this,
					   [this](FOnAttributeChangeData const& data)
					   {
						   OnHealthChanged.Broadcast(data.NewValue);
					   });

	HudPlayerParams.AbilitySystemComponent
		->GetGameplayAttributeValueChangeDelegate(GeoAttributeSet->GetMaxHealthAttribute())
		.AddWeakLambda(this,
					   [this](FOnAttributeChangeData const& data)
					   {
						   OnMaxHealthChanged.Broadcast(data.NewValue);
					   });
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoHUD::BeginPlay()
{
	Super::BeginPlay();

	// Auto-show boss health bar when enemies spawn
	if (UWorld* World = GetWorld())
	{
		if (AGeoGameState* GameState = World->GetGameState<AGeoGameState>())
		{
			GameState->OnEnemySpawned.AddDynamic(this, &AGeoHUD::ShowBossHealthBar);

			// If enemies already spawned before HUD was ready, show the first one
			if (AEnemyCharacter* FirstEnemy = GameState->GetFirstEnemy())
			{
				ShowBossHealthBar(FirstEnemy);
			}
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoHUD::ShowBossHealthBar(AEnemyCharacter* Boss)
{
	if (!Boss || !BossHealthBarWidgetClass)
	{
		return;
	}

	// Hide existing boss bar if showing a different boss
	HideBossHealthBar();

	if (!UHudFunctionLibrary::ShouldDrawHUD(GetOwner()))
	{
		return;
	}

	BossHealthBarWidget = CreateWidget<UGenericCombattantWidget>(GetWorld(), BossHealthBarWidgetClass);
	if (BossHealthBarWidget)
	{
		if (UAbilitySystemComponent* BossASC = Cast<IAbilitySystemInterface>(Boss)->GetAbilitySystemComponent())
		{
			BossHealthBarWidget->InitializeWithAbilitySystemComponent(BossASC);
		}
		BossHealthBarWidget->AddToViewport(10); // Higher Z-order to be on top
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoHUD::HideBossHealthBar()
{
	if (BossHealthBarWidget)
	{
		BossHealthBarWidget->RemoveFromParent();
		BossHealthBarWidget = nullptr;
	}
}
