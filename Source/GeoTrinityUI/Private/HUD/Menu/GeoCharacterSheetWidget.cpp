// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoCharacterSheetWidget.h"

#include "AbilitySystem/Abilities/Common/GeoDeployAbility.h"
#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/AttributeSet/GeoGemAttributeSet.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Characters/PlayableCharacter.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameClasses/GeoPlayerController.h"
#include "GameClasses/GeoPlayerState.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Gem/GeoGemCatalog.h"
#include "Gem/GeoGemProfileSave.h"
#include "Gem/GeoGemStatsEffect.h"
#include "Gem/GeoGemSubsystem.h"
#include "HUD/HudFunctionLibrary.h"
#include "HUD/Menu/GeoAbilityCardWidget.h"
#include "HUD/Menu/GeoAbilityDetailWidget.h"
#include "HUD/Menu/GeoGemsWidget.h"
#include "HUD/Menu/GeoMenuButton.h"
#include "HUD/Menu/GeoMenuRootWidget.h"
#include "HUD/Menu/GeoTableWidget.h"
#include "HUD/Style/GeoGemGlyph.h"
#include "HUD/Style/GeoMeter.h"
#include "HUD/Style/GeoShape.h"
#include "HUD/Style/GeoUITheme.h"
#include "TimerManager.h"

// ---------------------------------------------------------------------------------------------------------------------
UGeoCharacterSheetWidget::UGeoCharacterSheetWidget(FObjectInitializer const& ObjectInitializer) :
	Super(ObjectInitializer)
{
	Stats = {
		{INVTEXT("Max health"), UCharacterAttributeSet::GetMaxHealthAttribute(), EGeoStatFormat::Number},
		{INVTEXT("Shield"), UCharacterAttributeSet::GetShieldAttribute(), EGeoStatFormat::Number},
		{INVTEXT("Damage reduction"), UCharacterAttributeSet::GetDamageReductionAttribute(), EGeoStatFormat::Percent},
		{INVTEXT("Healing received"), UCharacterAttributeSet::GetReceivedHealBoostAttribute(),
		 EGeoStatFormat::Multiplier},
		{INVTEXT("Damage"), UCharacterAttributeSet::GetDamageMultiplierAttribute(), EGeoStatFormat::Multiplier},
		{INVTEXT("Healing done"), UCharacterAttributeSet::GetAppliedHealBoostAttribute(), EGeoStatFormat::Multiplier},
		{INVTEXT("Crit chance"), UGeoGemAttributeSet::GetCritChanceAttribute(), EGeoStatFormat::Percent},
		{INVTEXT("Crit damage"), UGeoGemAttributeSet::GetCritDamageAttribute(), EGeoStatFormat::Multiplier},
		{INVTEXT("Wind-up"), UGeoGemAttributeSet::GetWindUpMultiplierAttribute(), EGeoStatFormat::Multiplier},
		{INVTEXT("Reload speed"), UGeoGemAttributeSet::GetReloadSpeedMultiplierAttribute(), EGeoStatFormat::Multiplier},
		{INVTEXT("Max ammo"), UCharacterAttributeSet::GetMaxAmmoAttribute(), EGeoStatFormat::Number},
		{INVTEXT("Spell distance"), UGeoGemAttributeSet::GetSpellDistanceMultiplierAttribute(),
		 EGeoStatFormat::Multiplier},
		{INVTEXT("Move speed"), UCharacterAttributeSet::GetMovementSpeedMultiplierAttribute(),
		 EGeoStatFormat::Multiplier},
		{INVTEXT("Dash distance"), UGeoGemAttributeSet::GetDashDistanceMultiplierAttribute(),
		 EGeoStatFormat::Multiplier},
		{INVTEXT("Dash cooldown"), UGeoGemAttributeSet::GetDashCooldownMultiplierAttribute(),
		 EGeoStatFormat::Multiplier},
		{INVTEXT("Special cooldown"), UGeoGemAttributeSet::GetSpecialCooldownMultiplierAttribute(),
		 EGeoStatFormat::Multiplier},
		{INVTEXT("Deploy cooldown"), UGeoGemAttributeSet::GetDeployableCooldownMultiplierAttribute(),
		 EGeoStatFormat::Multiplier},
		{INVTEXT("Deploy health"), UGeoGemAttributeSet::GetDeployableHealthMultiplierAttribute(),
		 EGeoStatFormat::Multiplier},
		{INVTEXT("Deploy drain"), UGeoGemAttributeSet::GetDeployableDrainMultiplierAttribute(),
		 EGeoStatFormat::Multiplier},
		{INVTEXT("Deploy blink"), UGeoGemAttributeSet::GetDeployableBlinkMultiplierAttribute(),
		 EGeoStatFormat::Multiplier},
	};
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCharacterSheetWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (GemLoadoutButton)
	{
		GemLoadoutButton->OnClicked.AddUniqueDynamic(this, &UGeoCharacterSheetWidget::HandleGems);
	}

	GetWorld()->GetTimerManager().SetTimer(FightRefreshTimer, this, &UGeoCharacterSheetWidget::ShowFight,
										   FightRefreshInterval, true);

	if (UAbilitySystemComponent* ASC = GeoASLib::GetGeoAscFromActor(GetOwningPlayerState()))
	{
		for (FGeoSheetStat const& Stat : Stats)
		{
			ASC->GetGameplayAttributeValueChangeDelegate(Stat.Attribute)
				.AddUObject(this, &UGeoCharacterSheetWidget::HandleStatChanged);
		}
	}

	if (!IsInMenu())
	{
		OnPageShown();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCharacterSheetWidget::NativeDestruct()
{
	if (UAbilitySystemComponent* ASC = GeoASLib::GetGeoAscFromActor(GetOwningPlayerState()))
	{
		for (FGeoSheetStat const& Stat : Stats)
		{
			ASC->GetGameplayAttributeValueChangeDelegate(Stat.Attribute).RemoveAll(this);
		}
	}

	GetWorld()->GetTimerManager().ClearTimer(FightRefreshTimer);
	Super::NativeDestruct();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCharacterSheetWidget::OnPageShown()
{
	AGeoPlayerState const* PlayerState = GetOwningPlayerState<AGeoPlayerState>();
	APlayableCharacter const* PlayableCharacter = Cast<APlayableCharacter>(GetOwningPlayerPawn());
	UGeoGemSubsystem const* GemSubsystem = ULocalPlayer::GetSubsystem<UGeoGemSubsystem>(GetOwningLocalPlayer());
	UGeoGemProfileSave const* Profile = GemSubsystem ? GemSubsystem->GetProfile() : nullptr;
	if (PlayerState && PlayableCharacter)
	{
		EPlayerClass const PlayerClass = PlayerState->GetPlayerClass();
		ShowIdentity(PlayerClass, Profile);
		ShowStats();
		ShowFight();
		ShowAbilities(*PlayableCharacter);
		ShowGems(PlayerClass, Profile);
		CloseDetail();
	}
	else
	{
		UE_LOG(LogGeoTrinity, Log, TEXT("UGeoCharacterSheetWidget: no player state or playable pawn yet, sheet left empty"));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
UWidget* UGeoCharacterSheetWidget::GetInitialFocusWidget() const
{
	return AbilityBox && AbilityBox->HasAnyChildren() ? AbilityBox->GetChildAt(0) : Super::GetInitialFocusWidget();
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoCharacterSheetWidget::HandleBackAction()
{
	if (!SelectedRow)
	{
		return Super::HandleBackAction();
	}

	CloseDetail();
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
FReply UGeoCharacterSheetWidget::NativeOnMouseButtonDown(FGeometry const& InGeometry, FPointerEvent const& InMouseEvent)
{
	if (SelectedRow)
	{
		CloseDetail();
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCharacterSheetWidget::HandleAbilitySelected(UGeoAbilityCardWidget* Row)
{
	if (!ensureMsgf(DetailWidget, TEXT("%hs: no DetailWidget on %s"), __FUNCTION__, *GetName()))
	{
		return;
	}

	if (SelectedRow)
	{
		SelectedRow->SetSelected(false);
	}

	SelectedRow = Row;
	Row->SetSelected(true);
	DetailWidget->CopyAbility(*Row);
	DetailWidget->Open();
	if (DetailScrim)
	{
		DetailScrim->SetVisibility(ESlateVisibility::Visible);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCharacterSheetWidget::CloseDetail()
{
	if (SelectedRow)
	{
		SelectedRow->SetSelected(false);
	}

	SelectedRow = nullptr;
	if (DetailWidget)
	{
		DetailWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (DetailScrim)
	{
		DetailScrim->SetVisibility(ESlateVisibility::Collapsed);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCharacterSheetWidget::HandleGems()
{
	AGeoPlayerController* Controller = Cast<AGeoPlayerController>(GetOwningPlayer());
	if (IsInMenu())
	{
		OpenPage(UGeoGemsWidget::StaticClass());
	}
	else if (ensureMsgf(Controller, TEXT("%hs: %s is not owned by an AGeoPlayerController"), __FUNCTION__, *GetName()))
	{
		Controller->OpenPauseMenu();
		UGeoMenuRootWidget* PauseMenu = Cast<UGeoMenuRootWidget>(Controller->GetPauseMenuWidget());
		if (ensureMsgf(PauseMenu, TEXT("%hs: the pause menu is not a UGeoMenuRootWidget"), __FUNCTION__))
		{
			PauseMenu->OpenPage(UGeoGemsWidget::StaticClass());
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
UTextBlock* UGeoCharacterSheetWidget::MakeText(EGeoTextRole const Role, FText const& Text) const
{
	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	UGeoUITheme::ApplyTextStyle(TextBlock, Role);
	FSlateFontInfo Font = TextBlock->GetFont();
	Font.Size = RowTextSize;
	TextBlock->SetFont(Font);
	TextBlock->SetText(Text);
	return TextBlock;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCharacterSheetWidget::ShowIdentity(EPlayerClass const PlayerClass, UGeoGemProfileSave const* Profile)
{
	UGeoUITheme const* Theme = UGeoUITheme::Get();
	FGeoClassStyle const* ClassStyle = Theme ? Theme->FindClassStyle(PlayerClass) : nullptr;
	if (ClassStyle && ClassShape)
	{
		ClassShape->SetClassShape(*ClassStyle);
	}
	if (ClassStyle && ClassNameText)
	{
		ClassNameText->SetText(ClassStyle->Name);
	}
	for (UGeoMeter* Meter : {LevelPips.Get(), XpMeter.Get()})
	{
		if (ClassStyle && Meter)
		{
			Meter->SetFillTint(ClassStyle->Color);
		}
	}
	if (ClassStyle && SubtitleText)
	{
		APlayerState const* PlayerState = GetOwningPlayerState();
		SubtitleText->SetText(FText::Format(SubtitleFormat, ClassStyle->Role,
											FText::FromString(PlayerState ? PlayerState->GetPlayerName() : FString())));
	}

	UGeoGemCatalog const* Catalog = UGeoGemCatalog::Get();
	if (Profile && Catalog)
	{
		ShowLevel(PlayerClass, *Profile, *Catalog);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCharacterSheetWidget::ShowLevel(EPlayerClass const PlayerClass, UGeoGemProfileSave const& Profile,
										 UGeoGemCatalog const& Catalog)
{
	int32 const Level = Profile.GetClassLevel(PlayerClass);
	int32 const Xp = Profile.GetClassXp(PlayerClass);
	bool const bMaxLevel = Level >= GeoGem::MaxClassLevel;
	if (LevelText)
	{
		LevelText->SetText(FText::Format(LevelFormat, Level, GeoGem::MaxClassLevel));
	}
	if (LevelPips)
	{
		LevelPips->SetFill(static_cast<float>(Level) / GeoGem::MaxClassLevel);
	}
	if (XpMeter)
	{
		XpMeter->SetFill(bMaxLevel ? 1.f : static_cast<float>(Xp) / FMath::Max(Catalog.XpPerLevel, 1));
	}
	if (XpText)
	{
		XpText->SetText(FText::Format(XpFormat, Xp, Catalog.XpPerLevel));
	}
	if (NextLevelText)
	{
		int32 NextSockets = 0;
		for (FGeoGemSocket const& Socket : GeoGem::GetSockets())
		{
			NextSockets += Socket.UnlockLevel == Level + 1 ? 1 : 0;
		}
		NextLevelText->SetText(bMaxLevel ? MaxLevelText : FText::Format(NextLevelFormat, Level + 1, NextSockets));
	}

	UGeoUITheme const* Theme = UGeoUITheme::Get();
	if (OtherClassesBox && Theme)
	{
		OtherClassesBox->ClearChildren();
		for (TPair<EPlayerClass, FGeoClassStyle> const& Other : Theme->ClassStyles)
		{
			if (Other.Key != PlayerClass)
			{
				UGeoShape* Shape = WidgetTree->ConstructWidget<UGeoShape>(UGeoShape::StaticClass());
				Shape->Size = OtherClassShapeSize;
				Shape->SetClassShape(Other.Value);
				OtherClassesBox->AddChildToHorizontalBox(Shape)->SetVerticalAlignment(VAlign_Center);
				UTextBlock* OtherLevel = MakeText(
					EGeoTextRole::Mono, FText::Format(OtherClassLevelFormat, Profile.GetClassLevel(Other.Key)));
				UHorizontalBoxSlot* LevelSlot = OtherClassesBox->AddChildToHorizontalBox(OtherLevel);
				LevelSlot->SetPadding(FMargin(LabelGap, 0.f, ItemGap, 0.f));
				LevelSlot->SetVerticalAlignment(VAlign_Center);
			}
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCharacterSheetWidget::ShowStats()
{
	UAbilitySystemComponent* ASC = GeoASLib::GetGeoAscFromActor(GetOwningPlayerState());
	if (!StatTable || !ASC)
	{
		return;
	}

	FGameplayEffectQuery BuffQuery;
	BuffQuery.CustomMatchDelegate.BindLambda(
		[](FActiveGameplayEffect const& Effect)
		{
			return !Effect.Spec.Def->IsA<UGeoGemStatsEffect>();
		});
	TArray<FActiveGameplayEffectHandle> const BuffHandles = ASC->GetActiveEffects(BuffQuery);

	StatTable->ClearLines();
	for (FGeoSheetStat const& Stat : Stats)
	{
		float const Value = ASC->GetNumericAttribute(Stat.Attribute);
		float const WithGems = ASC->GetFilteredAttributeValue(Stat.Attribute, FGameplayTagRequirements(),
															  FGameplayTagContainer(), BuffHandles);
		FString ValueString;
		switch (Stat.Format)
		{
		case EGeoStatFormat::Number:
			ValueString = FString::FromInt(FMath::RoundToInt(Value));
			break;
		case EGeoStatFormat::Multiplier:
			ValueString = FString::Printf(TEXT("×%.2f"), Value);
			break;
		case EGeoStatFormat::Percent:
			ValueString = FString::Printf(TEXT("%.0f%%"), Value * 100.f);
			break;
		}

		StatTable->AddLine({StatTable->MakeCellText(Stat.Label), StatTable->MakeCellText(FText::FromString(ValueString)),
							MakeBonusCell(Stat.Format, WithGems - ASC->GetNumericAttributeBase(Stat.Attribute),
										  GemBonusColor),
							MakeBonusCell(Stat.Format, Value - WithGems, BuffBonusColor)});
	}

	for (FGameplayAbilitySpec const& Spec : ASC->GetActivatableAbilities())
	{
		if (UGeoDeployAbility const* Deploy = Cast<UGeoDeployAbility>(Spec.GetPrimaryInstance()))
		{
			StatTable->AddLine(
				{StatTable->MakeCellText(DeployStacksLabel), StatTable->MakeCellText(FText::AsNumber(Deploy->GetMaxStacks())),
				 MakeBonusCell(EGeoStatFormat::Number, Deploy->GetMaxStacks() - Deploy->GetBaseMaxStacks(), GemBonusColor),
				 MakeBonusCell(EGeoStatFormat::Number, 0.f, BuffBonusColor)});
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCharacterSheetWidget::HandleStatChanged(FOnAttributeChangeData const& /*Data*/)
{
	ShowStats();
}

// ---------------------------------------------------------------------------------------------------------------------
UTextBlock* UGeoCharacterSheetWidget::MakeBonusCell(EGeoStatFormat const Format, float const Bonus,
													FLinearColor const& Color) const
{
	UTextBlock* Cell = StatTable->MakeCellText(NoBonusText);
	Cell->SetColorAndOpacity(NoBonusColor);
	FNumberFormattingOptions Options;
	Options.SetAlwaysSign(true).SetMaximumFractionalDigits(2);
	if (!FMath::IsNearlyZero(Bonus) && Format == EGeoStatFormat::Number)
	{
		Cell->SetText(FText::AsNumber(FMath::RoundToInt(Bonus), &Options));
		Cell->SetColorAndOpacity(Color);
	}
	else if (!FMath::IsNearlyZero(Bonus))
	{
		Cell->SetText(FText::Format(BonusFormat, FText::AsNumber(Bonus * 100.f, &Options)));
		Cell->SetColorAndOpacity(Color);
	}

	return Cell;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCharacterSheetWidget::ShowFight()
{
	AGeoPlayerState const* PlayerState = GetOwningPlayerState<AGeoPlayerState>();
	if (!PlayerState || !FightTable)
	{
		return;
	}

	auto Figure = [this](float const Value) -> UWidget*
	{
		return FightTable->MakeCellText(UHudFunctionLibrary::FormatCompactNumber(Value));
	};
	auto NoFigure = [this]() -> UWidget*
	{
		UTextBlock* Cell = FightTable->MakeCellText(NoBonusText);
		Cell->SetColorAndOpacity(NoBonusColor);
		return Cell;
	};
	FightTable->ClearLines();
	FightTable->AddLine({FightTable->MakeCellText(FightDamageLabel), Figure(PlayerState->GetLiveDPS()),
						 Figure(PlayerState->GetFightDPS()), Figure(PlayerState->GetMaxBurstDamage()),
						 Figure(PlayerState->GetTotalDamageDealt())});
	FightTable->AddLine({FightTable->MakeCellText(FightHealingLabel), Figure(PlayerState->GetLiveHPS()),
						 Figure(PlayerState->GetFightHPS()), Figure(PlayerState->GetMaxBurstHealing()),
						 Figure(PlayerState->GetTotalHealingDealt())});
	FightTable->AddLine({FightTable->MakeCellText(FightTakenLabel), NoFigure(), NoFigure(), NoFigure(),
						 Figure(PlayerState->GetTotalDamageReceived())});
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCharacterSheetWidget::ShowAbilities(APlayableCharacter const& PlayableCharacter)
{
	if (!AbilityBox || !ensureMsgf(AbilityRowClass, TEXT("%hs: no AbilityRowClass on %s"), __FUNCTION__, *GetName()))
	{
		return;
	}

	AbilityBox->ClearChildren();
	for (UGeoAbilityCardWidget* Row : UGeoAbilityCardWidget::CreateClassCards(*this, AbilityRowClass, PlayableCharacter))
	{
		Row->SetIsFocusable(true);
		Row->OnSelected.AddUniqueDynamic(this, &UGeoCharacterSheetWidget::HandleAbilitySelected);
		AbilityBox->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, 0.f, 0.f, AbilityGap));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCharacterSheetWidget::ShowGems(EPlayerClass const PlayerClass, UGeoGemProfileSave const* Profile)
{
	UGeoGemCatalog const* Catalog = UGeoGemCatalog::Get();
	if (!Profile || !Catalog)
	{
		return;
	}

	int32 const Level = Profile->GetClassLevel(PlayerClass);
	TArray<FName> const& Slotted = Profile->GetLoadout(PlayerClass).Sockets;
	TArray<FGeoGemSocket> const& Sockets = GeoGem::GetSockets();
	int32 OpenCount = 0;
	int32 FilledCount = 0;
	TMap<FName, int32> SlottedCounts;
	for (int32 Index = 0; Index < Sockets.Num(); ++Index)
	{
		OpenCount += Sockets[Index].UnlockLevel <= Level ? 1 : 0;
		if (Slotted.IsValidIndex(Index) && !Slotted[Index].IsNone())
		{
			++FilledCount;
			++SlottedCounts.FindOrAdd(Slotted[Index]);
		}
	}

	if (SlottedText)
	{
		SlottedText->SetText(FText::Format(SlottedFormat, FilledCount, OpenCount));
	}
	if (GemBox)
	{
		GemBox->ClearChildren();
	}
	if (CoreEffectBox)
	{
		CoreEffectBox->ClearChildren();
	}
	for (TPair<FName, int32> const& SlottedGem : SlottedCounts)
	{
		FGeoGemInfo const* Gem = Catalog->Find(SlottedGem.Key);
		TOptional<EGeoGemTier> const Tier = Catalog->FindTier(SlottedGem.Key);
		if (Gem && Tier && GemBox)
		{
			UHorizontalBox* Chip = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			Chip->SetToolTipText(Gem->DisplayName);
			UGeoGemGlyph* Glyph = WidgetTree->ConstructWidget<UGeoGemGlyph>(UGeoGemGlyph::StaticClass());
			Glyph->SetSize(GemGlyphSize);
			Glyph->SetGem(*Tier, Gem->Color.GetColor(1.f));
			Chip->AddChildToHorizontalBox(Glyph)->SetVerticalAlignment(VAlign_Center);
			Chip->AddChildToHorizontalBox(MakeText(EGeoTextRole::Mono, FText::Format(GemCountFormat, SlottedGem.Value)))
				->SetPadding(FMargin(LabelGap, 0.f, ItemGap, LineGap));
			GemBox->AddChild(Chip);
		}
		if (Gem && Tier && *Tier == EGeoGemTier::Core && CoreEffectBox)
		{
			CoreEffectBox->AddChildToVerticalBox(MakeText(EGeoTextRole::Button, Gem->DisplayName));
			UTextBlock* Rule = MakeText(EGeoTextRole::Body, Gem->Effect);
			Rule->SetAutoWrapText(true);
			CoreEffectBox->AddChildToVerticalBox(Rule)->SetPadding(FMargin(0.f, LabelGap / 2.f, 0.f, LineGap));
		}
	}

	if (ShardsText)
	{
		ShardsText->SetText(FText::AsNumber(Profile->GetShards()));
	}
	if (OwnedText)
	{
		TArray<FFormatArgumentValue> OwnedPerTier;
		for (EGeoGemTier const Tier : {EGeoGemTier::Chip, EGeoGemTier::Cut, EGeoGemTier::Prism, EGeoGemTier::Core})
		{
			int32 Owned = 0;
			if (FGeoGemList const* List = Catalog->GemsByTier.Find(Tier))
			{
				for (FGeoGemInfo const& TierGem : List->Gems)
				{
					Owned += Profile->GetOwnedCount(TierGem.Id);
				}
			}
			OwnedPerTier.Add(Owned);
		}
		OwnedText->SetText(FText::Format(OwnedFormat, OwnedPerTier[0], OwnedPerTier[1], OwnedPerTier[2],
										 OwnedPerTier[3]));
	}
}
