// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/GeoHUD.h"

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
#include "AbilitySystem/Abilities/Circle/GeoSweetSpotChargePassiveAbility.h"
#include "AbilitySystem/Abilities/Common/GeoDeployAbility.h"
#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "AbilitySystem/Types/GeoAscTypes.h"
#include "AbilitySystemInterface.h"
#include "Algo/StableSort.h"
#include "Blueprint/UserWidget.h"
#include "Characters/Component/GeoDeployableManagerComponent.h"
#include "Characters/EnemyCharacter.h"
#include "Characters/PlayableCharacter.h"
#include "Characters/PlayerClassTypes.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Texture2D.h"
#include "GameClasses/GeoGameState.h"
#include "GameClasses/GeoPlayerController.h"
#include "GameClasses/GeoPlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "GameplayEffectExtension.h"
#include "HUD/GenericCombattantWidget.h"
#include "HUD/GeoDamageNumberWidget.h"
#include "HUD/GeoOverlayWidget.h"
#include "HUD/GeoUserWidget.h"
#include "HUD/HudFunctionLibrary.h"
#include "HUD/Interface/GeoDamageNumberHost.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"
#include "System/GeoCombatStatsSubsystem.h"
#include "Tool/UGeoGameplayLibrary.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	/** Left-to-right ability-bar position of an input tag, so the bar reads the same for every class and never follows
	 * the order abilities happened to be granted in. */
	int32 GetInputTagBarOrder(FGameplayTag const& InputTag)
	{
		FGeoGameplayTags const& Tags = FGeoGameplayTags::Get();
		TArray<FGameplayTag> const BarOrder = {Tags.InputTag_Basic, Tags.InputTag_Special,
											   Tags.InputTag_SpecialAlternative, Tags.InputTag_Reload,
											   Tags.InputTag_Dash};

		int32 const Order = BarOrder.IndexOfByKey(InputTag);
		ensureMsgf(Order != INDEX_NONE, TEXT("GetInputTagBarOrder: %s has no ability-bar position"),
				   *InputTag.ToString());
		return Order;
	}
} // namespace

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
	checkf(OverlayWidgetClass, TEXT("Overlay Widget Class uninitialized, please fill out HUD %s"), *GetName());

	// Setup params the HUD may very probably need to access
	HudPlayerParams.PlayerController = PC;
	HudPlayerParams.PlayerState = PS;
	HudPlayerParams.AbilitySystemComponent = ASC;
	HudPlayerParams.AttributeSet = AS;

	// AGeoPlayerState calls InitOverlay from both BeginPlay and OnPlayerPawnSet (replication-order safety net), so this
	// can run twice on the client. Build the overlay and bind delegates only once.
	if (!OverlayWidget && UHudFunctionLibrary::ShouldDrawHUD(GetOwner()))
	{
		// Owned by the controller, not the world: the slot widgets resolve their key labels through
		// GetOwningLocalPlayer(), which would otherwise read player 1's bindings for every couch-coop player.
		OverlayWidget = CreateWidget<UGeoUserWidget>(PC, OverlayWidgetClass);
		OverlayWidget->InitFromHUD(this);
		BroadcastInitialValues();
		BindCallbacksToDependencies();

		OverlayWidget->AddToViewport();

		if (UGeoOverlayWidget* Overlay = Cast<UGeoOverlayWidget>(OverlayWidget))
		{
			Overlay->InitStatusBar(this);
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
TArray<FGeoActiveEffectIcon> AGeoHUD::GetActiveEffectIcons() const
{
	TArray<FGeoActiveEffectIcon> Entries;

	UAbilitySystemComponent* ASC = HudPlayerParams.AbilitySystemComponent;
	if (!ASC)
	{
		return Entries;
	}

	float const WorldTime = ASC->GetActiveGameplayEffects().GetServerWorldTime();
	for (FActiveGameplayEffectHandle const& Handle : ASC->GetActiveEffects(FGameplayEffectQuery()))
	{
		FActiveGameplayEffect const* ActiveEffect = ASC->GetActiveGameplayEffect(Handle);
		if (!ActiveEffect)
		{
			continue;
		}

		FGeoGameplayEffectContext const* Context =
			static_cast<FGeoGameplayEffectContext const*>(ActiveEffect->Spec.GetContext().Get());
		if (!Context || !Context->GetIcon() || !ActiveEffect->Spec.Def)
		{
			continue;
		}

		FGeoActiveEffectIcon* Entry = Entries.FindByPredicate(
			[Context](FGeoActiveEffectIcon const& Candidate)
			{
				return Candidate.Icon == Context->GetIcon();
			});
		if (!Entry)
		{
			Entry = &Entries.AddDefaulted_GetRef();
			Entry->Icon = Context->GetIcon();
		}

		// A stacking GE is one active effect carrying its whole stack, an unstacked one is many effects of one stack
		// each, and the badge means the same thing in both cases.
		Entry->Count += ActiveEffect->Spec.GetStackCount();
		Entry->BoostBonus += GeoASLib::GetEffectBoostBonus(*ASC, ActiveEffect->Spec);
		float const TimeRemaining = ActiveEffect->GetTimeRemaining(WorldTime);
		if (TimeRemaining < 0.f || Entry->TimeRemaining < 0.f)
		{
			Entry->TimeRemaining = -1.f;
		}
		else if (TimeRemaining >= Entry->TimeRemaining)
		{
			Entry->TimeRemaining = TimeRemaining;
			Entry->Duration = ActiveEffect->GetDuration();
		}
	}

	UGeoSweetSpotChargePassiveAbility const* SweetSpotPassive =
		GeoASLib::GetGrantedAbility<UGeoSweetSpotChargePassiveAbility>(*ASC);
	if (SweetSpotPassive && SweetSpotPassive->GetGaugeIcon())
	{
		FGeoActiveEffectIcon& Entry = Entries.AddDefaulted_GetRef();
		Entry.Icon = SweetSpotPassive->GetGaugeIcon();
		Entry.Count = 1;
		Entry.TimeRemaining = -1.f;
		Entry.FillRatio = SweetSpotPassive->GetGaugeRatio(*ASC);
		Entry.FullColor = SweetSpotPassive->GetGaugeFullColor();
	}

	return Entries;
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoHUD::ForEachHudAttribute(
	TFunctionRef<void(FGameplayAttribute const&, FOnAttributeModifiedSignature const&)> Visitor) const
{
	UGeoAttributeSetBase const* GeoAttributeSet = HudPlayerParams.GetGeoAttributeSet();
	if (!GeoAttributeSet)
	{
		return;
	}

	Visitor(UGeoAttributeSetBase::GetHealthAttribute(), OnHealthChanged);
	Visitor(UGeoAttributeSetBase::GetMaxHealthAttribute(), OnMaxHealthChanged);
	Visitor(UGeoAttributeSetBase::GetShieldAttribute(), OnShieldChanged);

	// Ammo lives on the player-only attribute set, so these two rows exist for a playable character and nothing else.
	if (GeoAttributeSet->IsA<UCharacterAttributeSet>())
	{
		Visitor(UCharacterAttributeSet::GetAmmoAttribute(), OnAmmoChanged);
		Visitor(UCharacterAttributeSet::GetMaxAmmoAttribute(), OnMaxAmmoChanged);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoHUD::BroadcastInitialValues() const
{
	UGeoAttributeSetBase const* GeoAttributeSet = HudPlayerParams.GetGeoAttributeSet();
	ForEachHudAttribute(
		[GeoAttributeSet](FGameplayAttribute const& Attribute, FOnAttributeModifiedSignature const& Delegate)
		{
			Delegate.Broadcast(Attribute.GetNumericValue(GeoAttributeSet));
		});
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoHUD::BindCallbacksToDependencies()
{
	ForEachHudAttribute(
		[this](FGameplayAttribute const& Attribute, FOnAttributeModifiedSignature const& Delegate)
		{
			HudPlayerParams.AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Attribute).AddWeakLambda(
				this,
				[Target = &Delegate](FOnAttributeChangeData const& Data)
				{
					Target->Broadcast(Data.NewValue);
				});
		});
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoHUD::BindToPawn(APlayableCharacter* PlayableCharacter)
{
	// Called from AGeoPlayerState::OnPlayerPawnSet, the one client-side moment the pawn is guaranteed. Pawn-dependent
	// bindings live here, not in InitOverlay/BindCallbacksToDependencies, which run before the pawn may exist.
	UGeoDeployableManagerComponent* Manager =
		PlayableCharacter ? PlayableCharacter->GetComponentByClass<UGeoDeployableManagerComponent>() : nullptr;
	if (!ensureMsgf(Manager, TEXT("%hs: pawn has no UGeoDeployableManagerComponent on %s"), __FUNCTION__, *GetName()))
	{
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
	// One bar over one shared view: every couch-coop HUD would otherwise stack an identical copy on top.
	APlayerController const* OwningController = GetOwningPlayerController();
	ULocalPlayer const* LocalPlayer = OwningController ? OwningController->GetLocalPlayer() : nullptr;
	if (!Boss || !BossHealthBarWidgetClass || !LocalPlayer || LocalPlayer->GetLocalPlayerIndex() != 0)
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
		if (UAbilitySystemComponent* BossASC = Boss->GetAbilitySystemComponent())
		{
			BossHealthBarWidget->InitializeWithAbilitySystemComponent(BossASC);
		}
		BossHealthBarWidget->AddToViewport(10); // Higher Z-order to be on top
	}

	// The on-screen boss bar replaces the boss's floating bar.
	Boss->SetCombattantWidgetVisible(false);
	BossWithSuppressedBar = Boss;
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoHUD::HideBossHealthBar()
{
	if (BossHealthBarWidget)
	{
		BossHealthBarWidget->RemoveFromParent();
		BossHealthBarWidget = nullptr;
	}

	if (AEnemyCharacter* PrevBoss = BossWithSuppressedBar.Get())
	{
		PrevBoss->SetCombattantWidgetVisible(true);
	}
	BossWithSuppressedBar = nullptr;
}

// ---------------------------------------------------------------------------------------------------------------------
TArray<FGeoAbilityBarEntry> AGeoHUD::GetAbilityBarEntries(APlayableCharacter* PlayableCharacter) const
{
	TArray<FGeoAbilityBarEntry> Entries;

	UGeoAbilitySystemComponent* ASC = HudPlayerParams.GetGeoAbilitySystemComponent();
	UAbilityInfo const* AbilityInfo = UGeoAbilitySystemLibrary::GetAbilityInfo();
	if (!ensureMsgf(ASC && PlayableCharacter && AbilityInfo,
					TEXT("%hs: missing ASC, PlayableCharacter, or AbilityInfo on %s"), __FUNCTION__, *GetName()))
	{
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

	// Stable so abilities sharing one input (the slot's channel/detonate swap) keep the order the slot expects.
	Algo::StableSortBy(Entries,
					   [](FGeoAbilityBarEntry const& Entry)
					   {
						   return GetInputTagBarOrder(Entry.InputTag);
					   });

	return Entries;
}

// ---------------------------------------------------------------------------------------------------------------------
FGameplayAbilitySpec const* AGeoHUD::FindSpecForTag(FGameplayTag AbilityTag,
													UGeoGameplayAbility const*& OutAbility) const
{
	OutAbility = nullptr;

	UGeoAbilitySystemComponent const* ASC = HudPlayerParams.GetGeoAbilitySystemComponent();
	if (!ASC)
	{
		return nullptr;
	}

	for (FGameplayAbilitySpec const& Spec : ASC->GetActivatableAbilities())
	{
		UGeoGameplayAbility const* Ability = Cast<UGeoGameplayAbility>(Spec.Ability);
		if (!Ability || Ability->GetAbilityTag() != AbilityTag)
		{
			continue;
		}
		// Spec.Ability is the CDO for instanced abilities; hand back the active instance so per-instance state
		// (a fire-delay timer, deploy stacks) is read instead of the CDO's defaults.
		UGeoGameplayAbility const* const Instance = Cast<UGeoGameplayAbility>(Spec.GetPrimaryInstance());
		OutAbility = Instance ? Instance : Ability;
		return &Spec;
	}

	return nullptr;
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoHUD::GetAbilityCooldown(FGameplayTag AbilityTag, float& OutRemaining, float& OutDuration) const
{
	OutRemaining = 0.f;
	OutDuration = 0.f;

	UGeoGameplayAbility const* Ability = nullptr;
	if (FGameplayAbilitySpec const* Spec = FindSpecForTag(AbilityTag, Ability))
	{
		Ability->GetCooldownTimeRemainingAndDuration(
			Spec->Handle, HudPlayerParams.GetGeoAbilitySystemComponent()->AbilityActorInfo.Get(), OutRemaining,
			OutDuration);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
bool AGeoHUD::IsAbilityActive(FGameplayTag AbilityTag) const
{
	// Deliberately the spec and not the instance: "is this ability running" is spec state, and an instanced ability
	// with no live instance is simply not active.
	UGeoGameplayAbility const* Ability = nullptr;
	FGameplayAbilitySpec const* Spec = FindSpecForTag(AbilityTag, Ability);
	return Spec && Spec->IsActive();
}

// ---------------------------------------------------------------------------------------------------------------------
bool AGeoHUD::CanActivateAbility(FGameplayTag const AbilityTag) const
{
	UGeoGameplayAbility const* Ability = nullptr;
	FGameplayAbilitySpec const* Spec = FindSpecForTag(AbilityTag, Ability);
	return Spec
		&& Ability->CanActivateAbility(Spec->Handle,
									   HudPlayerParams.GetGeoAbilitySystemComponent()->AbilityActorInfo.Get());
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoHUD::GetDeployCountForAbility(FGameplayTag AbilityTag, int32& OutCurrent, int32& OutMax) const
{
	OutCurrent = 0;
	OutMax = 0;

	UGeoGameplayAbility const* Ability = nullptr;
	FindSpecForTag(AbilityTag, Ability);
	if (UGeoDeployAbility const* DeployAbility = Cast<UGeoDeployAbility>(Ability))
	{
		OutCurrent = DeployAbility->GetCurrentStacks();
		OutMax = DeployAbility->GetMaxStacks();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoHUD::RegisterASCForDamageNumbers(UAbilitySystemComponent* ASC, AActor* OwnerActor)
{
	if (!ASC || !IsValid(OwnerActor))
	{
		return;
	}

	if (IGeoDamageNumberHost const* NumberHost = Cast<IGeoDamageNumberHost>(OwnerActor);
		NumberHost && !NumberHost->ShowsDamageNumbers())
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
	bool const bShow =
		UGeoCombatStatsSubsystem::IsDebugDisplayEnabled() && Viewport && GameState && !GameState->PlayerArray.IsEmpty();
	if (!bShow)
	{
		RemoveCombatStatsPanel();
		return;
	}

	// Cell texts and colors poll their player state through Slate attributes, so the tree only needs rebuilding when
	// the player roster changes (not merely its count — a leave+join in the same count would otherwise go unnoticed).
	bool const bRosterChanged = CombatStatsRoster.Num() != GameState->PlayerArray.Num()
		|| [&]
		{
			for (int32 Index = 0; Index < CombatStatsRoster.Num(); ++Index)
			{
				if (CombatStatsRoster[Index].Get() != GameState->PlayerArray[Index])
				{
					return true;
				}
			}
			return false;
		}();
	if (CombatStatsPanel && !bRosterChanged)
	{
		return;
	}
	RemoveCombatStatsPanel();

	FSlateFontInfo const StatsFont = FCoreStyle::GetDefaultFontStyle("Regular", 12);
	constexpr float NameColumnWidth = 170.f;
	constexpr float StatColumnWidth = 48.f;

	auto MakeCell = [&StatsFont](float const Width, TAttribute<FText> Text, TAttribute<FSlateColor> Color)
	{
		return SNew(SBox).WidthOverride(Width)[SNew(STextBlock)
												   .Text(MoveTemp(Text))
												   .ColorAndOpacity(MoveTemp(Color))
												   .Font(StatsFont)
												   .OverflowPolicy(ETextOverflowPolicy::Ellipsis)];
	};

	TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);

	FSlateColor const HeaderColor = FLinearColor(0.8f, 0.8f, 0.8f, 1.f);
	TSharedRef<SHorizontalBox> HeaderRow = SNew(SHorizontalBox);
	HeaderRow->AddSlot().AutoWidth()[MakeCell(NameColumnWidth, FText::FromString(TEXT("Player")), HeaderColor)];
	for (TCHAR const* Label : {TEXT("DPS"), TEXT("MAX"), TEXT("Avg"), TEXT("Tot"), TEXT("HPS"), TEXT("MAX"),
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
				return FText::FromString(FString::Printf(TEXT("[%s] %s"),
														 GetPlayerClassStyle(WeakPlayerState->GetPlayerClass()).Label,
														 *WeakPlayerState->GetPlayerName()));
			});

		TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
		Row->AddSlot().AutoWidth()[MakeCell(NameColumnWidth, MoveTemp(NameText), RowColor)];

		using FStatGetter = float (AGeoPlayerState::*)() const;
		for (FStatGetter const Getter :
			 {&AGeoPlayerState::GetDebugDPS, &AGeoPlayerState::GetMaxBurstDamage, &AGeoPlayerState::GetFightDPS,
			  &AGeoPlayerState::GetTotalDamageDealt, &AGeoPlayerState::GetDebugHPS,
			  &AGeoPlayerState::GetMaxBurstHealing,
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
	CombatStatsRoster.Reset(GameState->PlayerArray.Num());
	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		CombatStatsRoster.Add(PlayerState);
	}

	// Anchored top-right, below the boss health bar which hugs the top edge of the screen.
	TSharedRef<SConstraintCanvas> Panel = SNew(SConstraintCanvas)
		+ SConstraintCanvas::Slot()
			  .Anchors(FAnchors(1.f, 0.08f))
			  .Alignment(FVector2D(1.f, 0.f))
			  .Offset(FMargin(-12.f, 0.f, 0.f, 0.f))
			  .AutoSize(true)[SNew(SBorder)
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
	CombatStatsRoster.Reset();
}
#endif
