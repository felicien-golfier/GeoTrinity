// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/Frame/GeoMessageLog.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Tool/Visual/GeoColor.h"
#include "Widgets/SLeafWidget.h"

/** Leaf widget painting the lines of its UGeoMessageLog; its active timer runs only while a message lives. */
class SGeoMessageLog : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SGeoMessageLog) {}
	SLATE_ARGUMENT(TWeakObjectPtr<UGeoMessageLog>, Owner)
	SLATE_END_ARGS()

	void Construct(FArguments const& InArgs)
	{
		Owner = InArgs._Owner;
		Wake();
	}

	/** Starts the timer when asleep. */
	void Wake()
	{
		if (!TimerHandle.IsValid())
		{
			TimerHandle = RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SGeoMessageLog::Step));
		}
	}

	virtual FVector2D ComputeDesiredSize(float /*LayoutScaleMultiplier*/) const override
	{
		UGeoMessageLog const* Log = Owner.Get();
		return FVector2D(0., Log ? Log->MaxLines * Log->LineHeight : 0.);
	}

	virtual int32 OnPaint(FPaintArgs const& /*Args*/, FGeometry const& AllottedGeometry,
						  FSlateRect const& /*MyCullingRect*/, FSlateWindowElementList& OutDrawElements, int32 LayerId,
						  FWidgetStyle const& InWidgetStyle, bool /*bParentEnabled*/) const override
	{
		UGeoMessageLog const* Log = Owner.Get();
		UGeoMessageSubsystem const* Messages = Log ? Log->GetMessages() : nullptr;
		UGeoUITheme const* Theme = Messages ? UGeoUITheme::Get() : nullptr;
		FGeoTextStyle const* TextStyle = Theme ? Theme->FindTextStyle(Log->TextRole) : nullptr;
		if (TextStyle)
		{
			float const Opacity = InWidgetStyle.GetColorAndOpacityTint().A;
			FVector2f const LocalSize = AllottedGeometry.GetLocalSize();
			OutDrawElements.PushClip(FSlateClippingZone(AllottedGeometry));
			for (FGeoMessageLine const& Line : UGeoMessageSubsystem::Layout(
					 Messages->GetMessages(), FPlatformTime::Seconds(), Log->GetLayoutParams()))
			{
				FGeoMessage const& Message = Messages->GetMessages()[Line.MessageIndex];
				FGeoColorParam ColorParam;
				ColorParam.Color = Message.Color;
				FVector2f const Offset(Log->LeftInset, LocalSize.Y - (Line.Slot + 1.f) * Log->LineHeight);
				FSlateDrawElement::MakeText(
					OutDrawElements, LayerId,
					AllottedGeometry.ToPaintGeometry(FVector2f(LocalSize.X - Log->LeftInset, Log->LineHeight),
													 FSlateLayoutTransform(Offset)),
					Message.Text.ToString(), TextStyle->Font, ESlateDrawEffect::None,
					ColorParam.GetColor(Line.Alpha * Opacity));
			}

			OutDrawElements.PopClip();
		}
		return LayerId;
	}

private:
	EActiveTimerReturnType Step(double /*CurrentTime*/, float /*DeltaTime*/)
	{
		UGeoMessageLog const* Log = Owner.Get();
		UGeoMessageSubsystem* Messages = Log ? Log->GetMessages() : nullptr;
		bool const bLive = Messages != nullptr;
		if (bLive)
		{
			Messages->Prune(FPlatformTime::Seconds(), Log->GetLayoutParams());
		}

		Invalidate(EInvalidateWidgetReason::Paint);
		bool const bContinue = bLive && !Messages->GetMessages().IsEmpty();
		if (!bContinue)
		{
			TimerHandle.Reset();
		}

		return bContinue ? EActiveTimerReturnType::Continue : EActiveTimerReturnType::Stop;
	}

	TWeakObjectPtr<UGeoMessageLog> Owner;
	TSharedPtr<FActiveTimerHandle> TimerHandle;
};

// ---------------------------------------------------------------------------------------------------------------------
UGeoMessageSubsystem* UGeoMessageLog::GetMessages() const
{
	UWorld const* World = GetWorld();
	UGameInstance const* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UGeoMessageSubsystem>() : nullptr;
}

// ---------------------------------------------------------------------------------------------------------------------
FGeoMessageLayoutParams UGeoMessageLog::GetLayoutParams() const
{
	return {MaxLines, Lifetime, FadeTime, SlideTime};
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMessageLog::ReleaseSlateResources(bool const bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	if (UGeoMessageSubsystem* Messages = GetMessages())
	{
		Messages->OnMessagePosted.Remove(PostedHandle);
	}

	PostedHandle.Reset();
	MyLog.Reset();
}

#if WITH_EDITOR
// ---------------------------------------------------------------------------------------------------------------------
FText const UGeoMessageLog::GetPaletteCategory()
{
	return INVTEXT("Geo");
}
#endif

// ---------------------------------------------------------------------------------------------------------------------
TSharedRef<SWidget> UGeoMessageLog::RebuildWidget()
{
	MyLog = SNew(SGeoMessageLog).Owner(this);
	if (UGeoMessageSubsystem* Messages = GetMessages())
	{
		PostedHandle = Messages->OnMessagePosted.AddSP(MyLog.ToSharedRef(), &SGeoMessageLog::Wake);
	}

	return MyLog.ToSharedRef();
}
