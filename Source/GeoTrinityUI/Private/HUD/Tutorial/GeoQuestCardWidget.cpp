// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Tutorial/GeoQuestCardWidget.h"

#include "Components/TextBlock.h"
#include "Engine/World.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoQuestCardWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::Collapsed);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoQuestCardWidget::ShowCard(FText const& Title, int32 const InMax, float const InTimeLimitSeconds)
{
	Max = InMax;
	TimeLimitSeconds = InTimeLimitSeconds;
	ShownTime = GetWorld()->GetTimeSeconds();
	TitleText->SetText(Title);
	SetCount(0);
	SetVisibility(ESlateVisibility::HitTestInvisible);
	if (TimerText)
	{
		TimerText->SetVisibility(TimeLimitSeconds > 0.f ? ESlateVisibility::HitTestInvisible
														: ESlateVisibility::Collapsed);
	}

	if (TimerText && TimeLimitSeconds > 0.f && !CountdownTimer.IsValid())
	{
		CountdownTimer = TakeWidget()->RegisterActiveTimer(
			0.f, FWidgetActiveTimerDelegate::CreateUObject(this, &UGeoQuestCardWidget::TickCountdown));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoQuestCardWidget::SetCount(int32 const Count)
{
	CountText->SetText(FText::Format(CountFormat, Count, Max));
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoQuestCardWidget::HideCard()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

// ---------------------------------------------------------------------------------------------------------------------
EActiveTimerReturnType UGeoQuestCardWidget::TickCountdown(double const /*CurrentTime*/, float /*DeltaTime*/)
{
	double const Left = FMath::Max(0., TimeLimitSeconds - (GetWorld()->GetTimeSeconds() - ShownTime));
	int32 const WholeSeconds = FMath::CeilToInt32(Left);
	TimerText->SetText(
		FText::Format(TimerFormat, WholeSeconds / 60,
					  FText::AsNumber(WholeSeconds % 60, &FNumberFormattingOptions().SetMinimumIntegralDigits(2))));

	return IsVisible() && WholeSeconds > 0 ? EActiveTimerReturnType::Continue : EActiveTimerReturnType::Stop;
}
