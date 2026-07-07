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
#include "Engine/GameViewportClient.h"
#include "GameClasses/GeoGameState.h"
#include "GameClasses/GeoPlayerController.h"
#include "GameClasses/GeoPlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "HUD/GenericCombattantWidget.h"
#include "HUD/GeoDamageNumberWidget.h"
#include "HUD/GeoOverlayWidget.h"
#include "HUD/GeoUserWidget.h"
#include "HUD/HudFunctionLibrary.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"
#include "System/GeoCombatStatsSubsystem.h"
#include "Tool/UGeoGameplayLibrary.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

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
bool AGeoHUD::CanActivateAbility(FGameplayTag const AbilityTag) const
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
			return Ability->CanActivateAbility(Spec.Handle, ASC->AbilityActorInfo.Get());
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

	OutCurrent = Manager->GetDeployables(DeployableClass).Num();

	int32 const* SlotCap = Manager->DeployableSlots.Find(DeployableClass);
	OutMax = SlotCap ? *SlotCap : Manager->GetMaxDeployables();
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoHUD::RegisterASCForDamageNumbers(UAbilitySystemComponent* ASC, AActor* OwnerActor)
{
	if (!ASC || !IsValid(OwnerActor))
	{
		return;
	}

	if (RegisteredDamageNumberASCs.Contains(ASC))
	{
		return;
	}
	RegisteredDamageNumberASCs.Add(ASC);

	TWeakObjectPtr<AActor> const WeakOwnerActor(OwnerActor);

	ASC->GetGameplayAttributeValueChangeDelegate(UGeoAttributeSetBase::GetHealthAttribute())
		.AddWeakLambda(this,
					   [this, WeakOwnerActor](FOnAttributeChangeData const& Data)
					   {
						   AActor* OwnerActor = WeakOwnerActor.Get();
						   float const Delta = Data.NewValue - Data.OldValue;
						   if (FMath::Abs(Delta) >= 0.5f && IsValid(OwnerActor))
						   {
							   SpawnDamageNumber(FMath::Abs(Delta), Delta > 0.f, OwnerActor->GetActorLocation());
						   }
					   });

	ASC->GetGameplayAttributeValueChangeDelegate(UGeoAttributeSetBase::GetShieldAttribute())
		.AddWeakLambda(this,
					   [this, WeakOwnerActor](FOnAttributeChangeData const& Data)
					   {
						   AActor* OwnerActor = WeakOwnerActor.Get();
						   float const Delta = Data.NewValue - Data.OldValue;
						   if (Delta < -0.5f && IsValid(OwnerActor))
						   {
							   SpawnDamageNumber(-Delta, false, OwnerActor->GetActorLocation());
						   }
					   });
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoHUD::SpawnDamageNumber(float Amount, bool bIsHeal, FVector WorldLocation)
{
	if (!DamageNumberWidgetClass)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC)
	{
		return;
	}

	UGeoDamageNumberWidget* Widget = nullptr;
	for (UGeoDamageNumberWidget* Candidate : DamageNumberPool)
	{
		if (IsValid(Candidate) && Candidate->IsAvailable())
		{
			Widget = Candidate;
			break;
		}
	}

	if (!Widget)
	{
		Widget = CreateWidget<UGeoDamageNumberWidget>(PC, DamageNumberWidgetClass);
		if (!ensureMsgf(Widget, TEXT("SpawnDamageNumber: CreateWidget failed on %s"), *GetName()))
		{
			return;
		}
		Widget->AddToViewport(5);
		Widget->SetVisibility(ESlateVisibility::Collapsed);
		DamageNumberPool.Add(Widget);
	}

	Widget->Activate(Amount, bIsHeal, WorldLocation);
}

#if !UE_BUILD_SHIPPING
namespace
{
// Compact display so the panel stays narrow once totals grow.
FText CompactNumber(float const Value)
{
	return FText::FromString(Value >= 1000.f ? FString::Printf(TEXT("%.1fk"), Value / 1000.f)
											 : FString::Printf(TEXT("%.0f"), Value));
}

struct FPlayerClassStyle
{
	TCHAR const* Label;
	FLinearColor Color;
};

FPlayerClassStyle GetPlayerClassStyle(EPlayerClass const PlayerClass)
{
	switch (PlayerClass)
	{
	case EPlayerClass::Triangle:
		return {TEXT("Tri"), FLinearColor(1.f, 0.35f, 0.35f, 1.f)};
	case EPlayerClass::Circle:
		return {TEXT("Cir"), FLinearColor(0.35f, 1.f, 0.35f, 1.f)};
	case EPlayerClass::Square:
		return {TEXT("Sqr"), FLinearColor(0.45f, 0.65f, 1.f, 1.f)};
	default:
		return {TEXT("?"), FLinearColor::White};
	}
}
} // namespace

// ---------------------------------------------------------------------------------------------------------------------
void AGeoHUD::DrawHUD()
{
	Super::DrawHUD();

	UpdateCombatStatsPanel();
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoHUD::EndPlay(EEndPlayReason::Type const EndPlayReason)
{
	RemoveCombatStatsPanel();
	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoHUD::UpdateCombatStatsPanel()
{
	AGameStateBase const* GameState = GetWorld()->GetGameState();
	UGameViewportClient* Viewport = GetWorld()->GetGameViewport();
	bool const bShow = UGeoCombatStatsSubsystem::IsDebugDisplayEnabled() && Viewport && GameState
					   && !GameState->PlayerArray.IsEmpty();
	if (!bShow)
	{
		RemoveCombatStatsPanel();
		return;
	}

	// Cell texts and colors poll their player state through Slate attributes, so the tree only needs rebuilding when
	// the player list changes.
	if (CombatStatsPanel && CombatStatsRowCount == GameState->PlayerArray.Num())
	{
		return;
	}
	RemoveCombatStatsPanel();

	FSlateFontInfo const StatsFont = FCoreStyle::GetDefaultFontStyle("Regular", 12);
	constexpr float NameColumnWidth = 170.f;
	constexpr float StatColumnWidth = 48.f;

	auto MakeCell = [&StatsFont](float const Width, TAttribute<FText> Text, TAttribute<FSlateColor> Color)
	{
		return SNew(SBox).WidthOverride(Width)
			[SNew(STextBlock)
				 .Text(MoveTemp(Text))
				 .ColorAndOpacity(MoveTemp(Color))
				 .Font(StatsFont)
				 .OverflowPolicy(ETextOverflowPolicy::Ellipsis)];
	};

	TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);

	FSlateColor const HeaderColor = FLinearColor(0.8f, 0.8f, 0.8f, 1.f);
	TSharedRef<SHorizontalBox> HeaderRow = SNew(SHorizontalBox);
	HeaderRow->AddSlot().AutoWidth()[MakeCell(NameColumnWidth, FText::FromString(TEXT("Player")), HeaderColor)];
	for (TCHAR const* Label : {TEXT("DPS"), TEXT("Best"), TEXT("Avg"), TEXT("Tot"), TEXT("HPS"), TEXT("Best"),
							   TEXT("Avg"), TEXT("Tot"), TEXT("Rcv")})
	{
		HeaderRow->AddSlot().AutoWidth()[MakeCell(StatColumnWidth, FText::FromString(Label), HeaderColor)];
	}
	Rows->AddSlot().AutoHeight()[HeaderRow];

	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		AGeoPlayerState const* GeoPlayerState = Cast<AGeoPlayerState>(PlayerState);
		if (!GeoPlayerState)
		{
			continue;
		}

		TWeakObjectPtr<AGeoPlayerState const> WeakPlayerState = GeoPlayerState;
		TAttribute<FSlateColor> const RowColor = TAttribute<FSlateColor>::CreateLambda(
			[WeakPlayerState]() -> FSlateColor
			{
				return WeakPlayerState.IsValid() ? GetPlayerClassStyle(WeakPlayerState->GetPlayerClass()).Color
												 : FLinearColor::White;
			});
		TAttribute<FText> NameText = TAttribute<FText>::CreateLambda(
			[WeakPlayerState]
			{
				if (!WeakPlayerState.IsValid())
				{
					return FText::GetEmpty();
				}
				return FText::FromString(FString::Printf(
					TEXT("[%s] %s"), GetPlayerClassStyle(WeakPlayerState->GetPlayerClass()).Label,
					*WeakPlayerState->GetPlayerName()));
			});

		TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
		Row->AddSlot().AutoWidth()[MakeCell(NameColumnWidth, MoveTemp(NameText), RowColor)];

		using FStatGetter = float (AGeoPlayerState::*)() const;
		for (FStatGetter const Getter :
			 {&AGeoPlayerState::GetDebugDPS, &AGeoPlayerState::GetBestDPS, &AGeoPlayerState::GetFightDPS,
			  &AGeoPlayerState::GetTotalDamageDealt, &AGeoPlayerState::GetDebugHPS, &AGeoPlayerState::GetBestHPS,
			  &AGeoPlayerState::GetFightHPS, &AGeoPlayerState::GetTotalHealingDealt,
			  &AGeoPlayerState::GetTotalDamageReceived})
		{
			TAttribute<FText> StatText = TAttribute<FText>::CreateLambda(
				[WeakPlayerState, Getter]
				{
					return WeakPlayerState.IsValid() ? CompactNumber((WeakPlayerState.Get()->*Getter)())
													 : FText::GetEmpty();
				});
			Row->AddSlot().AutoWidth()[MakeCell(StatColumnWidth, MoveTemp(StatText), RowColor)];
		}
		Rows->AddSlot().AutoHeight()[Row];
	}
	CombatStatsRowCount = GameState->PlayerArray.Num();

	// Anchored top-right, below the boss health bar which hugs the top edge of the screen.
	TSharedRef<SConstraintCanvas> Panel =
		SNew(SConstraintCanvas)
		+ SConstraintCanvas::Slot()
			  .Anchors(FAnchors(1.f, 0.08f))
			  .Alignment(FVector2D(1.f, 0.f))
			  .Offset(FMargin(-12.f, 0.f, 0.f, 0.f))
			  .AutoSize(true)
				  [SNew(SBorder)
					   .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
					   .BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.65f))
					   .Padding(FMargin(6.f, 4.f))[Rows]];

	Viewport->AddViewportWidgetContent(Panel);
	CombatStatsPanel = Panel;
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoHUD::RemoveCombatStatsPanel()
{
	if (!CombatStatsPanel)
	{
		return;
	}

	if (UGameViewportClient* Viewport = GetWorld()->GetGameViewport())
	{
		Viewport->RemoveViewportWidgetContent(CombatStatsPanel.ToSharedRef());
	}
	CombatStatsPanel.Reset();
}
#endif
