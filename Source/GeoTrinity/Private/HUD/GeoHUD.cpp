// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/GeoHUD.h"

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
#include "AbilitySystem/Abilities/Common/GeoDeployAbility.h"
#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystemInterface.h"
#include "Blueprint/UserWidget.h"
#include "Characters/Component/GeoDeployableManagerComponent.h"
#include "Characters/EnemyCharacter.h"
#include "Characters/PlayableCharacter.h"
#include "Characters/PlayerClassTypes.h"
#include "Engine/Canvas.h"
#include "GameClasses/GeoGameState.h"
#include "GameClasses/GeoPlayerController.h"
#include "GameClasses/GeoPlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "HUD/GenericCombattantWidget.h"
#include "HUD/GeoOverlayWidget.h"
#include "HUD/GeoUserWidget.h"
#include "HUD/HudFunctionLibrary.h"
#include "System/GeoCombatStatsSubsystem.h"
#include "Tool/UGeoGameplayLibrary.h"

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

		// Setup params the HUD may very probably need to access
		HudPlayerParams.PlayerController = PC;
	HudPlayerParams.PlayerState = PS;
	HudPlayerParams.AbilitySystemComponent = ASC;
	HudPlayerParams.AttributeSet = AS;

	// AGeoPlayerState calls InitOverlay from both BeginPlay and OnPlayerPawnSet (replication-order safety net), so this
	// can run twice on the client. Build the overlay and bind delegates only once.
	if (!OverlayWidget && UHudFunctionLibrary::ShouldDrawHUD(GetOwner()))
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
void AGeoHUD::BindToPawn(APlayableCharacter* PlayableCharacter)
{
	// Called from AGeoPlayerState::OnPlayerPawnSet, the one client-side moment the pawn is guaranteed. Pawn-dependent
	// bindings live here, not in InitOverlay/BindCallbacksToDependencies, which run before the pawn may exist.
	UGeoDeployableManagerComponent* Manager =
		PlayableCharacter ? PlayableCharacter->GetComponentByClass<UGeoDeployableManagerComponent>() : nullptr;
	if (!Manager)
	{
		ensureMsgf(Manager, TEXT("BindToPawn: pawn has no UGeoDeployableManagerComponent on %s"), *GetName());
		return;
	}

	// AddUnique so a re-possession of the same manager leaves exactly one binding.
	Manager->OnDeployCountChanged.AddUniqueDynamic(this, &AGeoHUD::HandleDeployCountChanged);

	// Build the ability bar now that the pawn's granted abilities exist (the overlay may have been created earlier,
	// before the pawn replicated).
	BuildAbilityBar(PlayableCharacter);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoHUD::BuildAbilityBar(APlayableCharacter* PlayableCharacter)
{
	if (UGeoOverlayWidget* Overlay = Cast<UGeoOverlayWidget>(OverlayWidget))
	{
		Overlay->BuildAbilityBar(this, PlayableCharacter);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoHUD::HandleDeployCountChanged(int32 /*CurrentCount*/, int32 /*MaxCount*/)
{
	// The manager's count is global; slots re-query their own per-ability count, so this is just a "refresh now" ping.
	OnPlayerDeployCountChanged.Broadcast();
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

	// The on-screen boss bar replaces the boss's floating bar.
	Boss->SetCombattantWidgetVisible(false);
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

// ---------------------------------------------------------------------------------------------------------------------
TArray<FGeoAbilityBarEntry> AGeoHUD::GetAbilityBarEntries(APlayableCharacter* PlayableCharacter) const
{
	TArray<FGeoAbilityBarEntry> Entries;

	UGeoAbilitySystemComponent* ASC = HudPlayerParams.GetGeoAbilitySystemComponent();
	UAbilityInfo const* AbilityInfo = UGeoAbilitySystemLibrary::GetAbilityInfo();
	if (!ASC || !PlayableCharacter || !AbilityInfo)
	{
		ensureMsgf(ASC && PlayableCharacter && AbilityInfo,
				   TEXT("GetAbilityBarEntries: missing ASC, PlayableCharacter, or AbilityInfo on %s"), *GetName());
		return Entries;
	}

	TArray<FPlayersGameplayAbilityInfo> const ClassInfos =
		AbilityInfo->GetAbilitiesForClass(PlayableCharacter->GetPlayerClass());

	for (FGameplayAbilitySpec const& Spec : ASC->GetActivatableAbilities())
	{
		UGeoGameplayAbility const* Ability = Cast<UGeoGameplayAbility>(Spec.Ability);
		if (!Ability || Ability->IsPassive())
		{
			continue;
		}

		FGameplayTag const AbilityTag = Ability->GetAbilityTag();
		FPlayersGameplayAbilityInfo const* Info = ClassInfos.FindByPredicate(
			[AbilityTag](FPlayersGameplayAbilityInfo const& Candidate)
			{
				return Candidate.AbilityTag == AbilityTag;
			});
		if (!Info)
		{
			continue;
		}

		FGeoAbilityBarEntry& Entry = Entries.AddDefaulted_GetRef();
		Entry.AbilityTag = AbilityTag;
		Entry.InputTag = Info->InputTag;
		Entry.InputAction = Info->InputAction;
		Entry.Icon = Info->AbilityIcon;
		Entry.bIsDeployable = Info->bShowDeployCount;
	}

	return Entries;
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoHUD::GetAbilityCooldown(FGameplayTag AbilityTag, float& OutRemaining, float& OutDuration) const
{
	OutRemaining = 0.f;
	OutDuration = 0.f;

	UGeoAbilitySystemComponent* ASC = HudPlayerParams.GetGeoAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	for (FGameplayAbilitySpec const& Spec : ASC->GetActivatableAbilities())
	{
		UGeoGameplayAbility const* Ability = Cast<UGeoGameplayAbility>(Spec.Ability);
		if (Ability && Ability->GetAbilityTag() == AbilityTag)
		{
			// Spec.Ability is the CDO for instanced abilities; query the active instance so per-instance state
			// (e.g. UGeoAutomaticFireAbility's fire-delay timer) is read instead of the CDO's empty timer.
			if (UGeoGameplayAbility const* Instance = Cast<UGeoGameplayAbility>(Spec.GetPrimaryInstance()))
			{
				Ability = Instance;
			}
			Ability->GetCooldownTimeRemainingAndDuration(Spec.Handle, ASC->AbilityActorInfo.Get(), OutRemaining,
														 OutDuration);
			return;
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
bool AGeoHUD::IsAbilityActive(FGameplayTag AbilityTag) const
{
	UGeoAbilitySystemComponent* ASC = HudPlayerParams.GetGeoAbilitySystemComponent();
	if (!ASC)
	{
		return false;
	}

	for (FGameplayAbilitySpec const& Spec : ASC->GetActivatableAbilities())
	{
		UGeoGameplayAbility const* Ability = Cast<UGeoGameplayAbility>(Spec.Ability);
		if (Ability && Ability->GetAbilityTag() == AbilityTag)
		{
			return Spec.IsActive();
		}
	}

	return false;
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoHUD::GetDeployCountForAbility(FGameplayTag AbilityTag, int32& OutCurrent, int32& OutMax) const
{
	OutCurrent = 0;
	OutMax = 0;

	UGeoAbilitySystemComponent* ASC = HudPlayerParams.GetGeoAbilitySystemComponent();
	APlayableCharacter const* PlayableCharacter = Cast<APlayableCharacter>(
		HudPlayerParams.PlayerController ? HudPlayerParams.PlayerController->GetPawn() : nullptr);
	if (!ASC || !PlayableCharacter)
	{
		return;
	}

	UGeoDeployableManagerComponent* Manager = PlayableCharacter->GetComponentByClass<UGeoDeployableManagerComponent>();
	if (!Manager)
	{
		return;
	}

	// Resolve the deployable class from the deploy ability so we can pick the matching manager slot.
	TSubclassOf<AGeoDeployableBase> DeployableClass = nullptr;
	for (FGameplayAbilitySpec const& Spec : ASC->GetActivatableAbilities())
	{
		UGeoDeployAbility const* DeployAbility = Cast<UGeoDeployAbility>(Spec.Ability);
		if (DeployAbility && DeployAbility->GetAbilityTag() == AbilityTag)
		{
			DeployableClass = DeployAbility->GetDeployableActorClass();
			break;
		}
	}
	if (!DeployableClass)
	{
		return;
	}

	OutCurrent = Manager->GetDeployables<AGeoDeployableBase>()
					 .FilterByPredicate(
						 [DeployableClass](AGeoDeployableBase const* Deployable)
						 {
							 return Deployable->IsA(DeployableClass);
						 })
					 .Num();

	int32 const* SlotCap = Manager->DeployableSlots.Find(DeployableClass);
	OutMax = SlotCap ? *SlotCap : Manager->GetMaxDeployables();
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
