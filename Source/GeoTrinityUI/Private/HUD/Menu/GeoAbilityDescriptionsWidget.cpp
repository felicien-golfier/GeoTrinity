// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoAbilityDescriptionsWidget.h"

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
#include "AbilitySystem/Abilities/Triangle/GeoReloadAbility.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "Algo/StableSort.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "Blueprint/WidgetTree.h"
#include "Characters/PlayableCharacter.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/RichTextBlock.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/DataTable.h"
#include "EnhancedInputSubsystems.h"
#include "HUD/Menu/GeoMenuButton.h"
#include "Styling/CoreStyle.h"

namespace
{
	float const AbilityIconSize = 128.f;
	FLinearColor const ValueColor(1.f, 0.78f, 0.25f);
	FLinearColor const PassiveColor(0.45f, 0.7f, 1.f);
	FLinearColor const KeybindColor(0.6f, 1.f, 0.6f);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilityDescriptionsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!BackButton)
	{
		ensureMsgf(BackButton, TEXT("UGeoAbilityDescriptionsWidget: BackButton is not bound"));
		return;
	}
	if (!AbilityList)
	{
		ensureMsgf(AbilityList, TEXT("UGeoAbilityDescriptionsWidget: AbilityList is not bound"));
		return;
	}

	BackButton->OnClicked.AddDynamic(this, &UGeoAbilityDescriptionsWidget::HandleBack);

	AbilityList->ClearChildren();
	BuildAbilityList();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilityDescriptionsWidget::NativeDestruct()
{
	if (BackButton)
	{
		BackButton->OnClicked.RemoveDynamic(this, &UGeoAbilityDescriptionsWidget::HandleBack);
	}
	Super::NativeDestruct();
}

// ---------------------------------------------------------------------------------------------------------------------
UWidget* UGeoAbilityDescriptionsWidget::GetInitialFocusWidget() const
{
	return BackButton;
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoAbilityDescriptionsWidget::HandleBackAction()
{
	HandleBack();
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilityDescriptionsWidget::HandleBack()
{
	OnClosed.Broadcast();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilityDescriptionsWidget::BuildAbilityList()
{
	APlayableCharacter* PlayableCharacter = Cast<APlayableCharacter>(GetOwningPlayerPawn());
	UAbilityInfo const* AbilityInfo = GeoASLib::GetAbilityInfo(this);
	if (!PlayableCharacter || !AbilityInfo)
	{
		UE_LOG(LogTemp, Log,
			   TEXT("UGeoAbilityDescriptionsWidget: no playable pawn or ability info yet, list left empty"));
		return;
	}
	UGeoAbilitySystemComponent const* ASC = GeoASLib::GetGeoAscFromActor(PlayableCharacter);

	UDataTable* StyleSet = NewObject<UDataTable>(this);
	StyleSet->RowStruct = FRichTextStyleRow::StaticStruct();
	FRichTextStyleRow DefaultRow;
	DefaultRow.TextStyle.SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 14));
	DefaultRow.TextStyle.SetColorAndOpacity(FSlateColor(FLinearColor::White));
	StyleSet->AddRow(FName(TEXT("Default")), DefaultRow);
	FRichTextStyleRow ValueRow = DefaultRow;
	ValueRow.TextStyle.SetColorAndOpacity(FSlateColor(ValueColor));
	StyleSet->AddRow(FName(TEXT("Value")), ValueRow);

	FGameplayTag const PassiveTag = FGeoGameplayTags::Get().Ability_Type_Passive;
	TArray<FPlayersGameplayAbilityInfo> ClassAbilities =
		AbilityInfo->GetAbilitiesForClass(PlayableCharacter->GetPlayerClass());
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
		UGeoGameplayAbility const* AbilityCDO = Cast<UGeoGameplayAbility>(Info.AbilityClass->GetDefaultObject());

		int32 AbilityLevel = 1;
		if (ASC)
		{
			for (FGameplayAbilitySpec const& Spec : ASC->GetActivatableAbilities())
			{
				if (GeoASLib::GetAbilityTagFromSpec(Spec) == Info.AbilityTag)
				{
					AbilityLevel = Spec.Level;
					break;
				}
			}
		}

		UHorizontalBox* RowBox = NewObject<UHorizontalBox>(WidgetTree);

		USizeBox* IconBox = NewObject<USizeBox>(WidgetTree);
		IconBox->SetWidthOverride(AbilityIconSize);
		IconBox->SetHeightOverride(AbilityIconSize);
		UImage* IconImage = NewObject<UImage>(WidgetTree);
		if (Info.AbilityIcon)
		{
			IconImage->SetBrushFromTexture(const_cast<UTexture2D*>(Info.AbilityIcon.Get()));
		}
		IconBox->AddChild(IconImage);
		UHorizontalBoxSlot* IconSlot = RowBox->AddChildToHorizontalBox(IconBox);
		IconSlot->SetPadding(FMargin(16.f));
		IconSlot->SetVerticalAlignment(VAlign_Center);

		UVerticalBox* TextBox = NewObject<UVerticalBox>(WidgetTree);
		UHorizontalBox* NameRow = NewObject<UHorizontalBox>(WidgetTree);

		UTextBlock* NameText = NewObject<UTextBlock>(WidgetTree);
		FSlateFontInfo NameFont = NameText->GetFont();
		NameFont.Size = 22;
		NameText->SetFont(NameFont);
		NameText->SetText(FText::FromString(Info.AbilityDisplayName));
		NameRow->AddChildToHorizontalBox(NameText);

		auto const AddNameAnnotation = [&](FString const& Annotation, FLinearColor const& Color)
		{
			UTextBlock* AnnotationText = NewObject<UTextBlock>(WidgetTree);
			FSlateFontInfo AnnotationFont = AnnotationText->GetFont();
			AnnotationFont.Size = 16;
			AnnotationText->SetFont(AnnotationFont);
			AnnotationText->SetColorAndOpacity(FSlateColor(Color));
			AnnotationText->SetText(FText::FromString(Annotation));
			UHorizontalBoxSlot* AnnotationSlot = NameRow->AddChildToHorizontalBox(AnnotationText);
			AnnotationSlot->SetPadding(FMargin(16.f, 0.f, 0.f, 2.f));
			AnnotationSlot->SetVerticalAlignment(VAlign_Bottom);
		};

		if (Info.TypeOfAbilityTag == PassiveTag)
		{
			AddNameAnnotation(TEXT("Passive"), PassiveColor);
		}

		if (Info.InputAction)
		{
			if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
					ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetOwningLocalPlayer()))
			{
				TArray<FKey> const Keys = InputSubsystem->QueryKeysMappedToAction(Info.InputAction);
				FKey const Key = Keys.Num() > 0 ? Keys[0] : FKey();
				if (Key.IsValid())
				{
					AddNameAnnotation(Key.GetDisplayName(false).ToString(), KeybindColor);
				}
			}
		}

		float const Cooldown = AbilityCDO ? AbilityCDO->GetCooldown(AbilityLevel) : 0.f;
		if (Cooldown > 0.f)
		{
			AddNameAnnotation(FString::Printf(TEXT("%gs cooldown"), Cooldown), ValueColor);
		}
		else if (AbilityCDO && AbilityCDO->GetFireDelay() > 0.f)
		{
			AddNameAnnotation(FString::Printf(TEXT("%gs delay"), AbilityCDO->GetFireDelay()), ValueColor);
		}
		TextBox->AddChildToVerticalBox(NameRow);

		UGeoReloadAbility const* ReloadCDO = Cast<UGeoReloadAbility>(AbilityCDO);
		FString DescriptionString = Info.GetResolvedDescription(AbilityLevel, true);

		TArray<FString> BuffLines;
		if (ReloadCDO)
		{
			int32 const NumBuffs = ReloadCDO->GetEffectDataArray().Num();
			TArray<FString> AllLines;
			DescriptionString.ParseIntoArrayLines(AllLines, false);
			if (NumBuffs > 0 && AllLines.Num() >= NumBuffs)
			{
				BuffLines.Append(AllLines.GetData() + AllLines.Num() - NumBuffs, NumBuffs);
				AllLines.SetNum(AllLines.Num() - NumBuffs);
				DescriptionString = FString::Join(AllLines, TEXT("\n"));
			}
		}

		URichTextBlock* DescriptionText = NewObject<URichTextBlock>(WidgetTree);
		DescriptionText->SetTextStyleSet(StyleSet);
		DescriptionText->SetAutoWrapText(true);
		DescriptionText->SetText(FText::FromString(DescriptionString));
		UVerticalBoxSlot* DescriptionSlot = TextBox->AddChildToVerticalBox(DescriptionText);
		DescriptionSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));

		for (int32 BuffIndex = 0; BuffIndex < BuffLines.Num(); ++BuffIndex)
		{
			UHorizontalBox* BuffRow = NewObject<UHorizontalBox>(WidgetTree);

			USizeBox* SwatchBox = NewObject<USizeBox>(WidgetTree);
			SwatchBox->SetWidthOverride(14.f);
			SwatchBox->SetHeightOverride(14.f);
			UBorder* Swatch = NewObject<UBorder>(WidgetTree);
			Swatch->SetBrushColor(ReloadCDO->GetColorForIndex(BuffIndex));
			SwatchBox->AddChild(Swatch);
			UHorizontalBoxSlot* SwatchSlot = BuffRow->AddChildToHorizontalBox(SwatchBox);
			SwatchSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
			SwatchSlot->SetVerticalAlignment(VAlign_Center);

			URichTextBlock* BuffText = NewObject<URichTextBlock>(WidgetTree);
			BuffText->SetTextStyleSet(StyleSet);
			BuffText->SetAutoWrapText(true);
			BuffText->SetText(FText::FromString(BuffLines[BuffIndex]));
			BuffRow->AddChildToHorizontalBox(BuffText);

			UVerticalBoxSlot* BuffRowSlot = TextBox->AddChildToVerticalBox(BuffRow);
			BuffRowSlot->SetPadding(FMargin(0.f, BuffIndex == 0 ? 4.f : 2.f, 0.f, 0.f));
		}

		UHorizontalBoxSlot* TextSlot = RowBox->AddChildToHorizontalBox(TextBox);
		TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TextSlot->SetPadding(FMargin(0.f, 16.f, 16.f, 16.f));
		TextSlot->SetVerticalAlignment(VAlign_Center);

		AbilityList->AddChild(RowBox);
	}
}
