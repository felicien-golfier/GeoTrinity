// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/GeoHUD.h"

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Blueprint/UserWidget.h"
#include "Characters/EnemyCharacter.h"
#include "Characters/PlayerClassTypes.h"
#include "Engine/Canvas.h"
#include "GameFramework/GameStateBase.h"
#include "GeoGameState.h"
#include "GeoPlayerController.h"
#include "GeoPlayerState.h"
#include "HUD/GenericCombattantWidget.h"
#include "HUD/GeoUserWidget.h"
#include "HUD/HudFunctionLibrary.h"
#include "System/GeoCombatStatsSubsystem.h"

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
	OnShieldChanged.Broadcast(GeoAttributeSet->GetShield());

	if (UCharacterAttributeSet const* CharacterAttributeSet = Cast<UCharacterAttributeSet>(GeoAttributeSet))
	{
		OnAmmoChanged.Broadcast(CharacterAttributeSet->GetAmmo());
		OnMaxAmmoChanged.Broadcast(CharacterAttributeSet->GetMaxAmmo());
	}
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

	HudPlayerParams.AbilitySystemComponent
		->GetGameplayAttributeValueChangeDelegate(GeoAttributeSet->GetShieldAttribute())
		.AddWeakLambda(this,
					   [this](FOnAttributeChangeData const& data)
					   {
						   OnShieldChanged.Broadcast(data.NewValue);
					   });

	if (UCharacterAttributeSet const* CharacterAttributeSet = Cast<UCharacterAttributeSet>(GeoAttributeSet))
	{
		HudPlayerParams.AbilitySystemComponent
			->GetGameplayAttributeValueChangeDelegate(CharacterAttributeSet->GetAmmoAttribute())
			.AddWeakLambda(this,
						   [this](FOnAttributeChangeData const& data)
						   {
							   OnAmmoChanged.Broadcast(data.NewValue);
						   });

		HudPlayerParams.AbilitySystemComponent
			->GetGameplayAttributeValueChangeDelegate(CharacterAttributeSet->GetMaxAmmoAttribute())
			.AddWeakLambda(this,
						   [this](FOnAttributeChangeData const& data)
						   {
							   OnMaxAmmoChanged.Broadcast(data.NewValue);
						   });
	}
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

#if !UE_BUILD_SHIPPING
// ---------------------------------------------------------------------------------------------------------------------
void AGeoHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!UGeoCombatStatsSubsystem::IsDebugDisplayEnabled() || !Canvas)
	{
		return;
	}

	AGameStateBase const* GameState = GetWorld()->GetGameState();
	if (!GameState)
	{
		return;
	}

	TArray<AGeoPlayerState const*> PlayerStates;
	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		if (AGeoPlayerState const* GeoPlayerState = Cast<AGeoPlayerState>(PlayerState))
		{
			PlayerStates.Add(GeoPlayerState);
		}
	}

	if (PlayerStates.IsEmpty())
	{
		return;
	}

	constexpr float PanelWidth = 600.f;
	constexpr float RowHeight = 18.f;
	constexpr float PanelPaddingX = 8.f;
	constexpr float PanelPaddingY = 8.f;
	float const PanelHeight = PanelPaddingY + (PlayerStates.Num() + 1) * RowHeight + PanelPaddingY;
	float const PanelX = Canvas->SizeX - PanelWidth - 16.f;
	constexpr float PanelY = 16.f;

	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.65f), PanelX, PanelY, PanelWidth, PanelHeight);

	// Column offsets relative to PanelX
	constexpr float ColPlayer = PanelPaddingX;
	constexpr float ColDPS = 180.f;
	constexpr float ColHPS = 240.f;
	constexpr float ColRecv = 300.f;
	constexpr float ColTotDmg = 370.f;
	constexpr float ColTotHeal = 450.f;
	constexpr float ColTotRecv = 530.f;

	float CurY = PanelY + PanelPaddingY;
	constexpr FLinearColor HeaderColor(0.8f, 0.8f, 0.8f, 1.f);

	DrawText(TEXT("Player"), HeaderColor, PanelX + ColPlayer, CurY);
	DrawText(TEXT("DPS"), HeaderColor, PanelX + ColDPS, CurY);
	DrawText(TEXT("HPS"), HeaderColor, PanelX + ColHPS, CurY);
	DrawText(TEXT("Recv/s"), HeaderColor, PanelX + ColRecv, CurY);
	DrawText(TEXT("TotDmg"), HeaderColor, PanelX + ColTotDmg, CurY);
	DrawText(TEXT("TotHeal"), HeaderColor, PanelX + ColTotHeal, CurY);
	DrawText(TEXT("TotRecv"), HeaderColor, PanelX + ColTotRecv, CurY);
	CurY += RowHeight;

	for (AGeoPlayerState const* GeoPlayerState : PlayerStates)
	{
		FLinearColor RowColor;
		FString ClassName;
		switch (GeoPlayerState->GetPlayerClass())
		{
		case EPlayerClass::Triangle:
			ClassName = TEXT("Tri");
			RowColor = FLinearColor(1.f, 0.35f, 0.35f, 1.f);
			break;
		case EPlayerClass::Circle:
			ClassName = TEXT("Cir");
			RowColor = FLinearColor(0.35f, 1.f, 0.35f, 1.f);
			break;
		case EPlayerClass::Square:
			ClassName = TEXT("Sqr");
			RowColor = FLinearColor(0.45f, 0.65f, 1.f, 1.f);
			break;
		default:
			ClassName = TEXT("?");
			RowColor = FLinearColor::White;
			break;
		}

		FString const PlayerLabel = FString::Printf(TEXT("[%s] %s"), *ClassName, *GeoPlayerState->GetPlayerName());
		DrawText(PlayerLabel, RowColor, PanelX + ColPlayer, CurY);
		DrawText(FString::Printf(TEXT("%.1f"), GeoPlayerState->GetDebugDPS()), RowColor, PanelX + ColDPS, CurY);
		DrawText(FString::Printf(TEXT("%.1f"), GeoPlayerState->GetDebugHPS()), RowColor, PanelX + ColHPS, CurY);
		DrawText(FString::Printf(TEXT("%.1f"), GeoPlayerState->GetDebugRecv()), RowColor, PanelX + ColRecv, CurY);
		DrawText(FString::Printf(TEXT("%.0f"), GeoPlayerState->GetTotalDamageDealt()), RowColor, PanelX + ColTotDmg,
				 CurY);
		DrawText(FString::Printf(TEXT("%.0f"), GeoPlayerState->GetTotalHealingDealt()), RowColor, PanelX + ColTotHeal,
				 CurY);
		DrawText(FString::Printf(TEXT("%.0f"), GeoPlayerState->GetTotalDamageReceived()), RowColor, PanelX + ColTotRecv,
				 CurY);
		CurY += RowHeight;
	}
}
#endif
