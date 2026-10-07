// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Style/GeoFrame.h"

#include "Brushes/SlateNoResource.h"
#include "Components/BorderSlot.h"
#include "HUD/Style/GeoFrameStyle.h"
#include "HUD/Style/GeoShapePainter.h"
#include "HUD/Style/GeoUITheme.h"
#include "Widgets/Layout/SBorder.h"

// Paint order inside a frame, each on its own layer so Slate's batching never reorders them: fill, grid, then the glow,
// line and runners of the styled outline over it, then the content.
static int32 const FillLayer = 0;
static int32 const GridLayer = 1;
static int32 const LineLayer = 2;
static int32 const ContentLayer = 5;

/** SBorder painting its UGeoFrameStyle under its content and easing between idle and active on an active timer. */
class SGeoFrame : public SBorder
{
public:
	SLATE_BEGIN_ARGS(SGeoFrame) {}
	SLATE_END_ARGS()

	void Construct(FArguments const& /*InArgs*/)
	{
		SBorder::Construct(SBorder::FArguments());
		// Each frame starts its runners somewhere else, so a column of buttons never marches in step.
		State.Travel = FMath::FRandRange(0.0, 4096.0);
		RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SGeoFrame::Animate));
	}

	void SetFrameStyle(UGeoFrameStyle const* InFrameStyle)
	{
		FrameStyle = InFrameStyle;
		Invalidate(EInvalidateWidgetReason::Paint);
	}

	void SetForcedActive(bool const bInForcedActive)
	{
		bForcedActive = bInForcedActive;
	}

	void SetActivateOnHoverAndFocus(bool const bInActivate)
	{
		bActivateOnHoverAndFocus = bInActivate;
	}

	virtual int32 OnPaint(FPaintArgs const& Args, FGeometry const& AllottedGeometry, FSlateRect const& MyCullingRect,
						  FSlateWindowElementList& OutDrawElements, int32 LayerId, FWidgetStyle const& InWidgetStyle,
						  bool bParentEnabled) const override
	{
		if (UGeoFrameStyle const* Style = FrameStyle.Get())
		{
			float const EnabledOpacity = ShouldBeEnabled(bParentEnabled) ? 1.f : Style->DisabledOpacity;
			float const Opacity = InWidgetStyle.GetColorAndOpacityTint().A * EnabledOpacity;
			TArray<FVector2f> const Outline = FGeoShapePainter::MakeCutRectangle(
				AllottedGeometry.GetLocalSize(), .5f * Style->LineThickness, Style->CornerCut);

			FGeoShapePainter::PaintStyledFill(*Style, State, Opacity, Outline, AllottedGeometry, OutDrawElements,
											  LayerId + FillLayer);
			if (Style->GridSpacing > 0.f)
			{
				FLinearColor GridColor = Style->GridColor;
				GridColor.A *= Opacity;
				PaintGrid(Style->GridSpacing, GridColor, AllottedGeometry, OutDrawElements, LayerId + GridLayer);
			}
			FGeoShapePainter::PaintStyledLine(*Style, State, Opacity, Outline, AllottedGeometry, OutDrawElements,
											  LayerId + LineLayer);
		}
		return SBorder::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId + ContentLayer,
								InWidgetStyle, bParentEnabled);
	}

private:
	bool IsActive() const
	{
		return bForcedActive || (bActivateOnHoverAndFocus && (IsHovered() || HasFocusedDescendants()));
	}

	EActiveTimerReturnType Animate(double /*CurrentTime*/, float const DeltaTime)
	{
		if (UGeoFrameStyle const* Style = FrameStyle.Get())
		{
			FGeoShapePainter::AdvanceStyledState(*Style, IsActive(), DeltaTime, State);
			Invalidate(EInvalidateWidgetReason::Paint);
		}
		return EActiveTimerReturnType::Continue;
	}

	static void PaintGrid(float const Spacing, FLinearColor const& Color, FGeometry const& Geometry,
						  FSlateWindowElementList& OutDrawElements, int32 const LayerId)
	{
		FVector2f const Size = Geometry.GetLocalSize();
		for (float X = Spacing; X < Size.X; X += Spacing)
		{
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(),
										 TArray<FVector2f>{{X, 0.f}, {X, Size.Y}}, ESlateDrawEffect::None, Color, true,
										 1.f);
		}
		for (float Y = Spacing; Y < Size.Y; Y += Spacing)
		{
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(),
										 TArray<FVector2f>{{0.f, Y}, {Size.X, Y}}, ESlateDrawEffect::None, Color, true,
										 1.f);
		}
	}

	TWeakObjectPtr<UGeoFrameStyle const> FrameStyle;
	bool bForcedActive = false;
	bool bActivateOnHoverAndFocus = false;
	FGeoOutlineState State;
};

// ---------------------------------------------------------------------------------------------------------------------
UGeoFrame::UGeoFrame(FObjectInitializer const& ObjectInitializer) : Super(ObjectInitializer)
{
	SetBrush(FSlateNoResource());
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoFrame::SetActive(bool const bInActive)
{
	bActive = bInActive;
	if (MyFrame)
	{
		MyFrame->SetForcedActive(bActive);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoFrame::SetFrameStyle(UGeoFrameStyle* InFrameStyle)
{
	FrameStyle = InFrameStyle;
	SynchronizeProperties();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoFrame::SetActivateOnHoverAndFocus(bool const bInActivate)
{
	bActivateOnHoverAndFocus = bInActivate;
	if (MyFrame)
	{
		MyFrame->SetActivateOnHoverAndFocus(bActivateOnHoverAndFocus);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoFrame::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (MyFrame)
	{
		UGeoFrameStyle const* Style = FrameStyle;
		UGeoUITheme const* Theme = Style ? nullptr : UGeoUITheme::Get();
		if (Theme)
		{
			Style = Theme->DefaultFrameStyle;
			ensureMsgf(Style, TEXT("%hs: %s has no DefaultFrameStyle"), __FUNCTION__, *Theme->GetName());
		}
		MyFrame->SetFrameStyle(Style);
		MyFrame->SetForcedActive(bActive);
		MyFrame->SetActivateOnHoverAndFocus(bActivateOnHoverAndFocus);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoFrame::ReleaseSlateResources(bool const bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyFrame.Reset();
}

#if WITH_EDITOR
// ---------------------------------------------------------------------------------------------------------------------
FText const UGeoFrame::GetPaletteCategory()
{
	return INVTEXT("Geo");
}
#endif

// ---------------------------------------------------------------------------------------------------------------------
TSharedRef<SWidget> UGeoFrame::RebuildWidget()
{
	MyFrame = SNew(SGeoFrame);
	MyBorder = MyFrame;
	if (GetChildrenCount() > 0)
	{
		Cast<UBorderSlot>(GetContentSlot())->BuildSlot(MyBorder.ToSharedRef());
	}
	return MyFrame.ToSharedRef();
}
