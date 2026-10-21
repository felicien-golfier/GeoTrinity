// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Tutorial/GeoShakeTextDecorator.h"

#include "HUD/Tutorial/GeoCatLineMarkup.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Text/STextBlock.h"

/** A text run that moves around its place on a Slate active timer. */
class SGeoShakeText : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGeoShakeText) {}
	SLATE_ARGUMENT(FText, Text)
	SLATE_ARGUMENT(FTextBlockStyle, TextStyle)
	SLATE_ARGUMENT(float, Amplitude)
	SLATE_ARGUMENT(float, Frequency)
	SLATE_END_ARGS()

	void Construct(FArguments const& InArgs)
	{
		Amplitude = InArgs._Amplitude;
		Frequency = InArgs._Frequency;
		TextStyle = InArgs._TextStyle;
		ChildSlot[SNew(STextBlock).Text(InArgs._Text).TextStyle(&TextStyle)];
		RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SGeoShakeText::Shake));
	}

private:
	EActiveTimerReturnType Shake(double const CurrentTime, float /*DeltaTime*/)
	{
		float const Time = static_cast<float>(CurrentTime) * Frequency;
		SetRenderTransform(FSlateRenderTransform(
			FVector2f(FMath::PerlinNoise1D(Time), FMath::PerlinNoise1D(Time + 31.7f)) * Amplitude));
		return EActiveTimerReturnType::Continue;
	}

	FTextBlockStyle TextStyle;
	float Amplitude = 0.f;
	float Frequency = 0.f;
};

/** Hands every <Shake> run to an SGeoShakeText. */
class FGeoShakeDecorator : public FRichTextDecorator
{
public:
	FGeoShakeDecorator(URichTextBlock* InOwner, UGeoShakeTextDecorator const& InSettings) :
		FRichTextDecorator(InOwner), Amplitude(InSettings.Amplitude), Frequency(InSettings.Frequency)
	{
	}

	virtual bool Supports(FTextRunParseResults const& RunParseResult, FString const& /*Text*/) const override
	{
		return RunParseResult.Name == GeoCatLine::ShakeRunName;
	}

protected:
	virtual TSharedPtr<SWidget> CreateDecoratorWidget(FTextRunInfo const& RunInfo,
													  FTextBlockStyle const& DefaultTextStyle) const override
	{
		return SNew(SGeoShakeText)
			.Text(RunInfo.Content)
			.TextStyle(DefaultTextStyle)
			.Amplitude(Amplitude)
			.Frequency(Frequency);
	}

private:
	float Amplitude;
	float Frequency;
};

// ---------------------------------------------------------------------------------------------------------------------
TSharedPtr<ITextDecorator> UGeoShakeTextDecorator::CreateDecorator(URichTextBlock* InOwner)
{
	return MakeShared<FGeoShakeDecorator>(InOwner, *this);
}
