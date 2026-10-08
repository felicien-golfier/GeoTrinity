// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoAbilityDetailWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/RichTextBlock.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Framework/Application/SlateApplication.h"
#include "HUD/Style/GeoFrame.h"
#include "HUD/Style/GeoUITheme.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilityDetailWidget::SetAbility(FPlayersGameplayAbilityInfo const& Info, int32 const AbilityLevel,
										 FLinearColor const& ClassColor)
{
	Super::SetAbility(Info, AbilityLevel, ClassColor);

	StatGrid->ClearChildren();
	TArray<FGeoAbilityStat> const Stats = Info.GetResolvedStats(AbilityLevel, true);
	for (int32 StatIndex = 0; StatIndex < Stats.Num(); ++StatIndex)
	{
		UGeoFrame* Cell = WidgetTree->ConstructWidget<UGeoFrame>();
		Cell->SetFrameStyle(StatFrameStyle);
		Cell->SetPadding(StatPadding);
		UVerticalBox* CellBody = WidgetTree->ConstructWidget<UVerticalBox>();
		Cell->SetContent(CellBody);

		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
		UGeoUITheme::ApplyTextStyle(Label, EGeoTextRole::Label);
		Label->SetText(FText::FromString(Stats[StatIndex].Label));
		Label->SetAutoWrapText(true);
		CellBody->AddChildToVerticalBox(Label);

		URichTextBlock* Value = WidgetTree->ConstructWidget<URichTextBlock>();
		Value->SetTextStyleSet(DescriptionText->GetTextStyleSet());
		Value->SetText(FText::FromString(Stats[StatIndex].Value));
		Value->SetAutoWrapText(true);
		CellBody->AddChildToVerticalBox(Value);

		UUniformGridSlot* CellSlot =
			StatGrid->AddChildToUniformGrid(Cell, StatIndex / StatColumns, StatIndex % StatColumns);
		CellSlot->SetHorizontalAlignment(HAlign_Fill);
		CellSlot->SetVerticalAlignment(VAlign_Fill);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilityDetailWidget::Open()
{
	if (!IsVisible())
	{
		SetVisibility(ESlateVisibility::Visible);
		SlideStartTime = FSlateApplication::Get().GetCurrentTime();
		ForceLayoutPrepass();
		SetRenderTranslation(FVector2D(GetDesiredSize().X, 0.f));
		TakeWidget()->RegisterActiveTimer(
			0.f, FWidgetActiveTimerDelegate::CreateUObject(this, &UGeoAbilityDetailWidget::Slide));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
EActiveTimerReturnType UGeoAbilityDetailWidget::Slide(double const CurrentTime, float /*DeltaTime*/)
{
	float const Alpha =
		SlideDuration > 0.f ? FMath::Clamp(static_cast<float>(CurrentTime - SlideStartTime) / SlideDuration, 0.f, 1.f)
							: 1.f;
	SetRenderTranslation(FVector2D(GetDesiredSize().X * (1.f - FMath::InterpEaseOut(0.f, 1.f, Alpha, 3.f)), 0.f));
	return Alpha < 1.f ? EActiveTimerReturnType::Continue : EActiveTimerReturnType::Stop;
}

