// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoCharacterSheetWidget.h"

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
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
#include "GameClasses/GeoPlayerState.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Gem/GeoGemCatalog.h"
#include "Gem/GeoGemProfileSave.h"
#include "Gem/GeoGemSubsystem.h"
#include "HUD/HudFunctionLibrary.h"
#include "HUD/Menu/GeoAbilityCardWidget.h"
#include "HUD/Menu/GeoMenuButton.h"
#include "HUD/Style/GeoGemGlyph.h"
#include "HUD/Style/GeoMeter.h"
#include "HUD/Style/GeoShape.h"
#include "HUD/Style/GeoUITheme.h"

// ---------------------------------------------------------------------------------------------------------------------
UGeoCharacterSheetWidget::UGeoCharacterSheetWidget(FObjectInitializer const& ObjectInitializer) :
	Super(ObjectInitializer)
{
	Stats = {
		{INVTEXT("Max health"), UCharacterAttributeSet::GetMaxHealthAttribute(), EGeoStatFormat::Number},
		{INVTEXT("Shield"), UCharacterAttributeSet::GetShieldAttribute(), EGeoStatFormat::Number},
		{INVTEXT("Damage"), UCharacterAttributeSet::GetDamageMultiplierAttribute(), EGeoStatFormat::Multiplier},
		{INVTEXT("Damage reduction"), UCharacterAttributeSet::GetDamageReductionAttribute(), EGeoStatFormat::Percent},
		{INVTEXT("Healing done"), UCharacterAttributeSet::GetAppliedHealBoostAttribute(), EGeoStatFormat::Multiplier},
		{INVTEXT("Healing received"), UCharacterAttributeSet::GetReceivedHealBoostAttribute(),
		 EGeoStatFormat::Multiplier},
		{INVTEXT("Move speed"), UCharacterAttributeSet::GetMovementSpeedMultiplierAttribute(),
		 EGeoStatFormat::Multiplier},
		{INVTEXT("Max ammo"), UCharacterAttributeSet::GetMaxAmmoAttribute(), EGeoStatFormat::Number},
	};
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCharacterSheetWidget::NativeConstruct()
{
	Super::NativeConstruct();

	AbilityDetailsButton->OnClicked.AddUniqueDynamic(this, &UGeoCharacterSheetWidget::HandleAbilityDetails);
	if (GemLoadoutButton)
	{
		GemLoadoutButton->OnClicked.AddUniqueDynamic(this, &UGeoCharacterSheetWidget::HandleGems);
	}
	Refresh();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCharacterSheetWidget::Refresh()
{
	AGeoPlayerState const* PlayerState = GetOwningPlayerState<AGeoPlayerState>();
	APlayableCharacter const* PlayableCharacter = Cast<APlayableCharacter>(GetOwningPlayerPawn());
	UGeoGemSubsystem const* GemSubsystem = ULocalPlayer::GetSubsystem<UGeoGemSubsystem>(GetOwningLocalPlayer());
	UGeoGemProfileSave const* Profile = GemSubsystem ? GemSubsystem->GetProfile() : nullptr;
	if (PlayerState && PlayableCharacter)
	{
		EPlayerClass const PlayerClass = PlayerState->GetPlayerClass();
		ShowIdentity(PlayerClass, Profile);
		ShowStats(PlayerClass, Profile);
		ShowFight();
		ShowAbilities(*PlayableCharacter);
		ShowGems(PlayerClass, Profile);
	}
	else
	{
		UE_LOG(LogGeoTrinity, Log, TEXT("UGeoCharacterSheetWidget: no player state or playable pawn yet, sheet left empty"));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
UWidget* UGeoCharacterSheetWidget::GetInitialFocusWidget() const
{
	return AbilityDetailsButton;
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoCharacterSheetWidget::HandleBackAction()
{
	HandleBack();
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCharacterSheetWidget::HandleBack()
{
	OnClosed.Broadcast();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCharacterSheetWidget::HandleAbilityDetails()
{
	OnOpenAbilityDetails.Broadcast();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCharacterSheetWidget::HandleGems()
{
	OnOpenGems.Broadcast();
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
void UGeoCharacterSheetWidget::ShowStats(EPlayerClass const PlayerClass, UGeoGemProfileSave const* Profile)
{
	AGeoPlayerState const* PlayerState = GetOwningPlayerState<AGeoPlayerState>();
	UAbilitySystemComponent const* ASC = PlayerState ? PlayerState->GetAbilitySystemComponent() : nullptr;
	UGeoGemCatalog const* Catalog = UGeoGemCatalog::Get();
	if (!StatBox || !ASC)
	{
		return;
	}

	TArray<FName> const Slotted = Profile ? Profile->GetLoadout(PlayerClass).Sockets : TArray<FName>();
	StatBox->ClearChildren();
	for (FGeoSheetStat const& Stat : Stats)
	{
		float const Value = ASC->GetNumericAttribute(Stat.Attribute);
		FString ValueString;
		switch (Stat.Format)
		{
		case EGeoStatFormat::Number:
			ValueString = FString::FromInt(FMath::RoundToInt(Value));
			break;
		case EGeoStatFormat::Multiplier:
			ValueString = FString::Printf(TEXT("\u00D7%.2f"), Value);
			break;
		case EGeoStatFormat::Percent:
			ValueString = FString::Printf(TEXT("%.0f%%"), Value * 100.f);
			break;
		}

		float GemBonus = 0.f;
		for (FName const GemId : Slotted)
		{
			FGeoGemInfo const* Gem = Catalog && !GemId.IsNone() ? Catalog->Find(GemId) : nullptr;
			GemBonus += Gem && Gem->Attribute == Stat.Attribute ? Gem->MagnitudePerGem : 0.f;
		}
		FNumberFormattingOptions BonusOptions;
		BonusOptions.SetAlwaysSign(true).SetMaximumFractionalDigits(2);
		FText const BonusText = GemBonus != 0.f
			? FText::Format(BonusFormat, FText::AsNumber(GemBonus * 100.f, &BonusOptions))
			: NoBonusText;

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Row->AddChildToHorizontalBox(MakeText(EGeoTextRole::Body, Stat.Label))
			->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		Row->AddChildToHorizontalBox(MakeText(EGeoTextRole::Mono, FText::FromString(ValueString)));
		UTextBlock* Bonus = MakeText(EGeoTextRole::Mono, BonusText);
		Bonus->SetColorAndOpacity(GemBonus != 0.f ? GemBonusColor : NoBonusColor);
		Bonus->SetMinDesiredWidth(BonusColumnWidth);
		Bonus->SetJustification(ETextJustify::Right);
		Row->AddChildToHorizontalBox(Bonus)->SetPadding(FMargin(ItemGap, 0.f, 0.f, 0.f));
		StatBox->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, 0.f, 0.f, LineGap));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCharacterSheetWidget::ShowFight()
{
	AGeoPlayerState const* PlayerState = GetOwningPlayerState<AGeoPlayerState>();
	if (!PlayerState)
	{
		return;
	}

	TPair<UTextBlock*, float> const FightValues[] = {{FightDpsText.Get(), PlayerState->GetFightDPS()},
													 {FightHpsText.Get(), PlayerState->GetFightHPS()},
													 {FightTakenText.Get(), PlayerState->GetTotalDamageReceived()}};
	for (TPair<UTextBlock*, float> const& FightValue : FightValues)
	{
		if (FightValue.Key)
		{
			FightValue.Key->SetText(UHudFunctionLibrary::FormatCompactNumber(FightValue.Value));
		}
	}
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
			CoreEffectBox->AddChildToVerticalBox(MakeText(EGeoTextRole::Body, Gem->DisplayName))
				->SetPadding(FMargin(0.f, 0.f, 0.f, LineGap));
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
