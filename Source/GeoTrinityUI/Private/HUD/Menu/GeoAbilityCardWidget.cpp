// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoAbilityCardWidget.h"

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
#include "AbilitySystem/Abilities/Triangle/GeoReloadAbility.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "Algo/StableSort.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/RichTextBlock.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Characters/PlayableCharacter.h"
#include "Framework/Application/SlateApplication.h"
#include "HUD/Style/GeoFrame.h"
#include "HUD/Style/GeoIconImage.h"
#include "HUD/Style/GeoUITheme.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilityCardWidget::SetAbility(FPlayersGameplayAbilityInfo const& Info, int32 const AbilityLevel,
									   FLinearColor const& ClassColor)
{
	Ability = Info;
	Level = AbilityLevel;
	Color = ClassColor;

	bool const bPassive = Info.TypeOfAbilityTag == FGeoGameplayTags::Get().Ability_Type_Passive;
	UGeoGameplayAbility const* AbilityCDO =
		Info.AbilityClass ? Cast<UGeoGameplayAbility>(Info.AbilityClass->GetDefaultObject()) : nullptr;

	Icon->SetIcon(Info.AbilityIcon);
	if (IconFrame)
	{
		IconFrame->SetGlowTint(bPassive ? PassiveColor : ClassColor);
	}
	NameText->SetText(FText::FromString(Info.AbilityDisplayName));

	KeyText->SetText(GetSlotLabel(Info));
	if (bPassive)
	{
		KeyText->SetColorAndOpacity(FSlateColor(PassiveColor));
	}

	float const Cooldown = AbilityCDO ? AbilityCDO->GetCooldown(AbilityLevel) : 0.f;
	float const FireDelay = AbilityCDO ? AbilityCDO->GetFireDelay() : 0.f;
	FText const TimingLabel = Cooldown > 0.f ? CooldownLabel : DelayLabel;
	float const Timing = Cooldown > 0.f ? Cooldown : FireDelay;
	TimingText->SetText(Timing > 0.f ? FText::FromString(FString::Printf(TEXT("%s <Value>%gs</>"),
																		 *TimingLabel.ToString(), Timing))
									 : FText::GetEmpty());

	FString Description = Info.GetResolvedDescription(AbilityLevel, true);

	// The Reload's last description lines are its buffs, one per effect, each shown beside its buff colour.
	UGeoReloadAbility const* ReloadCDO = Cast<UGeoReloadAbility>(AbilityCDO);
	TArray<FString> BuffLines;
	if (ReloadCDO && BuffBox)
	{
		int32 const BuffCount = ReloadCDO->GetEffectDataArray().Num();
		TArray<FString> Lines;
		Description.ParseIntoArrayLines(Lines, false);
		if (BuffCount > 0 && Lines.Num() >= BuffCount)
		{
			BuffLines.Append(Lines.GetData() + Lines.Num() - BuffCount, BuffCount);
			Lines.SetNum(Lines.Num() - BuffCount);
			Description = FString::Join(Lines, TEXT("\n"));
		}
	}
	DescriptionText->SetText(FText::FromString(Description));

	if (BuffBox)
	{
		BuffBox->ClearChildren();
		for (int32 BuffIndex = 0; BuffIndex < BuffLines.Num(); ++BuffIndex)
		{
			UHorizontalBox* BuffRow = WidgetTree->ConstructWidget<UHorizontalBox>();

			USizeBox* SwatchBox = WidgetTree->ConstructWidget<USizeBox>();
			SwatchBox->SetWidthOverride(BuffSwatchSize);
			SwatchBox->SetHeightOverride(BuffSwatchSize);
			UBorder* Swatch = WidgetTree->ConstructWidget<UBorder>();
			Swatch->SetBrushColor(ReloadCDO->GetColorForIndex(BuffIndex));
			SwatchBox->AddChild(Swatch);
			UHorizontalBoxSlot* SwatchSlot = BuffRow->AddChildToHorizontalBox(SwatchBox);
			SwatchSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
			SwatchSlot->SetVerticalAlignment(VAlign_Center);

			URichTextBlock* BuffText = WidgetTree->ConstructWidget<URichTextBlock>();
			BuffText->SetTextStyleSet(DescriptionText->GetTextStyleSet());
			BuffText->SetAutoWrapText(true);
			BuffText->SetText(FText::FromString(BuffLines[BuffIndex]));
			BuffRow->AddChildToHorizontalBox(BuffText)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

			BuffBox->AddChildToVerticalBox(BuffRow)->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilityCardWidget::CopyAbility(UGeoAbilityCardWidget const& Source)
{
	SetAbility(Source.Ability, Source.Level, Source.Color);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilityCardWidget::SetSelected(bool const bSelected)
{
	if (CardFrame)
	{
		CardFrame->SetActive(bSelected);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
FReply UGeoAbilityCardWidget::NativeOnMouseButtonDown(FGeometry const& InGeometry, FPointerEvent const& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		return FReply::Handled().CaptureMouse(TakeWidget());
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

// ---------------------------------------------------------------------------------------------------------------------
FReply UGeoAbilityCardWidget::NativeOnMouseButtonUp(FGeometry const& InGeometry, FPointerEvent const& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && HasMouseCapture())
	{
		if (InGeometry.IsUnderLocation(InMouseEvent.GetScreenSpacePosition()))
		{
			OnSelected.Broadcast(this);
		}
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

// ---------------------------------------------------------------------------------------------------------------------
FReply UGeoAbilityCardWidget::NativeOnKeyDown(FGeometry const& InGeometry, FKeyEvent const& InKeyEvent)
{
	if (FSlateApplication::Get().GetNavigationActionFromKey(InKeyEvent) == EUINavigationAction::Accept)
	{
		OnSelected.Broadcast(this);
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

// ---------------------------------------------------------------------------------------------------------------------
FText UGeoAbilityCardWidget::GetSlotLabel(FPlayersGameplayAbilityInfo const& Info) const
{
	FGeoGameplayTags const& Tags = FGeoGameplayTags::Get();
	FGameplayTag const& Type = Info.TypeOfAbilityTag;
	FText const* Label = nullptr;
	if (Type == Tags.Ability_Type_Passive)
	{
		Label = &PassiveLabel;
	}
	else if (Type == Tags.Ability_Type_Basic)
	{
		Label = &BasicLabel;
	}
	else if (Type == Tags.Ability_Type_Deployable)
	{
		Label = &DeployableLabel;
	}
	else if (Type == Tags.Ability_Type_Special)
	{
		Label = &SpecialLabel;
	}
	else if (Type == Tags.Ability_Type_Reload)
	{
		Label = &ReloadLabel;
	}
	else if (Type == Tags.Ability_Type_Dash)
	{
		Label = &DashLabel;
	}

	ensureMsgf(Label, TEXT("%hs: %s has no slot label"), __FUNCTION__, *Type.ToString());
	return Label ? *Label : FText::GetEmpty();
}

// ---------------------------------------------------------------------------------------------------------------------
TArray<UGeoAbilityCardWidget*> UGeoAbilityCardWidget::CreateClassCards(UUserWidget& Owner,
																		TSubclassOf<UGeoAbilityCardWidget> CardClass,
																		APlayableCharacter const& PlayableCharacter)
{
	TArray<UGeoAbilityCardWidget*> Cards;
	UAbilityInfo const* AbilityInfo = GeoASLib::GetAbilityInfo();
	UGeoUITheme const* Theme = UGeoUITheme::Get();
	if (!ensureMsgf(CardClass && AbilityInfo && Theme, TEXT("%hs: no card class, ability info or theme"), __FUNCTION__))
	{
		return Cards;
	}

	EPlayerClass const PlayerClass = PlayableCharacter.GetPlayerClass();
	FGeoClassStyle const* ClassStyle = Theme->FindClassStyle(PlayerClass);
	FLinearColor const ClassColor = ClassStyle ? ClassStyle->Color : FLinearColor::White;
	UGeoAbilitySystemComponent const* ASC = Cast<UGeoAbilitySystemComponent>(PlayableCharacter.GetAbilitySystemComponent());

	FGameplayTag const PassiveTag = FGeoGameplayTags::Get().Ability_Type_Passive;
	TArray<FPlayersGameplayAbilityInfo> ClassAbilities = AbilityInfo->GetAbilitiesForClass(PlayerClass);
	Algo::StableSortBy(ClassAbilities,
					   [PassiveTag](FPlayersGameplayAbilityInfo const& Info)
					   {
						   return Info.TypeOfAbilityTag == PassiveTag;
					   });

	for (FPlayersGameplayAbilityInfo const& Info : ClassAbilities)
	{
		if (!Info.AbilityClass)
		{
			continue;
		}

		int32 AbilityLevel = 1;
		bool bGranted = false;
		if (ASC)
		{
			for (FGameplayAbilitySpec const& Spec : ASC->GetActivatableAbilities())
			{
				if (GeoASLib::GetAbilityTagFromSpec(Spec) == Info.AbilityTag)
				{
					AbilityLevel = Spec.Level;
					bGranted = true;
					break;
				}
			}
		}

		if (Info.bGiveAtStartup || bGranted)
		{
			UGeoAbilityCardWidget* Card = CreateWidget<UGeoAbilityCardWidget>(&Owner, CardClass);
			Card->SetAbility(Info, AbilityLevel, ClassColor);
			Cards.Add(Card);
		}
	}
	return Cards;
}
