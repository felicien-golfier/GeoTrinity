// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/Frame/GeoMenuGuideWidget.h"

#include "Framework/Application/SlateApplication.h"
#include "GameClasses/Player/GeoPlayerController.h"
#include "HUD/HudFunctionLibrary.h"
#include "HUD/Menu/Frame/GeoMenuPageWidget.h"
#include "HUD/Menu/Frame/GeoMenuRootWidget.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

/** Leaf widget drawing the pulsing outline around the guide's hole. It owns the guide's per-frame timer: Slate time, so
 * it runs while the solo pause holds the world. */
class SGeoGuideFrame : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SGeoGuideFrame) {}
	SLATE_ARGUMENT(TWeakObjectPtr<UGeoMenuGuideWidget>, Owner)
	SLATE_END_ARGS()

	void Construct(FArguments const& InArgs)
	{
		SetVisibility(EVisibility::HitTestInvisible);
		Owner = InArgs._Owner;
		Wake();
	}

	/** Starts the timer when it sleeps; it sleeps again once the guide has no goal. */
	void Wake()
	{
		if (!HasActiveTimers())
		{
			RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SGeoGuideFrame::Step));
		}
	}

	/** Hole: the outline's rectangle in this widget's space; unset draws nothing. */
	void Set(TOptional<FSlateRect> const& InHole, FLinearColor const& InColor, float const InThickness,
			 float const InPulseSeconds, float const InPulseReach)
	{
		Hole = InHole;
		Color = InColor;
		Thickness = InThickness;
		PulseSeconds = InPulseSeconds;
		PulseReach = InPulseReach;
	}

	virtual FVector2D ComputeDesiredSize(float /*LayoutScaleMultiplier*/) const override
	{
		return FVector2D::ZeroVector;
	}

	virtual int32 OnPaint(FPaintArgs const& /*Args*/, FGeometry const& AllottedGeometry,
						  FSlateRect const& /*MyCullingRect*/, FSlateWindowElementList& OutDrawElements, int32 LayerId,
						  FWidgetStyle const& /*InWidgetStyle*/, bool /*bParentEnabled*/) const override
	{
		if (Hole.IsSet())
		{
			float const Time = static_cast<float>(FSlateApplication::Get().GetCurrentTime());
			float const Phase = .5f + .5f * FMath::Sin(2.f * PI * Time / PulseSeconds);
			DrawOutline(OutDrawElements, LayerId, AllottedGeometry, 0.f, Color);
			FLinearColor Swell = Color;
			Swell.A *= (1.f - Phase) * .7f;
			DrawOutline(OutDrawElements, LayerId + 1, AllottedGeometry, Phase * PulseReach, Swell);
		}

		return LayerId + 1;
	}

private:
	void DrawOutline(FSlateWindowElementList& OutDrawElements, int32 const LayerId, FGeometry const& Geometry,
					 float const Grow, FLinearColor const& Tint) const
	{
		FSlateRect const Rect = Hole.GetValue().ExtendBy(FMargin(Grow));
		TArray<FVector2f> const Corners = {FVector2f(Rect.Left, Rect.Top), FVector2f(Rect.Right, Rect.Top),
										   FVector2f(Rect.Right, Rect.Bottom), FVector2f(Rect.Left, Rect.Bottom),
										   FVector2f(Rect.Left, Rect.Top)};
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(), Corners,
									 ESlateDrawEffect::None, Tint, true, Thickness);
	}

	EActiveTimerReturnType Step(double /*CurrentTime*/, float /*DeltaTime*/)
	{
		UGeoMenuGuideWidget* const Guide = Owner.Get();
		bool const bGuiding = Guide && Guide->IsGuiding();
		if (bGuiding)
		{
			Guide->Refresh();
		}

		Invalidate(EInvalidateWidgetReason::Paint);
		return bGuiding ? EActiveTimerReturnType::Continue : EActiveTimerReturnType::Stop;
	}

	TWeakObjectPtr<UGeoMenuGuideWidget> Owner;
	TOptional<FSlateRect> Hole;
	FLinearColor Color = FLinearColor::White;
	float Thickness = 2.f;
	float PulseSeconds = 1.f;
	float PulseReach = 8.f;
};

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMenuGuideWidget::StartGuide(EGeoMenuGuideGoal const InGoal)
{
	Goal = InGoal;
	if (Frame.IsValid())
	{
		Frame->Wake();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMenuGuideWidget::StopGuide()
{
	Goal = EGeoMenuGuideGoal::None;
	PreparedPage.Reset();
	PreparedGoal = EGeoMenuGuideGoal::None;
	Rebuild(TOptional<FSlateRect>(), FText::GetEmpty());
}

// ---------------------------------------------------------------------------------------------------------------------
FGeoMenuGuideTarget UGeoMenuGuideWidget::FindTarget(APlayerController* const Controller,
													EGeoMenuGuideGoal const InGoal) const
{
	FGeoMenuGuideTarget Target;
	AGeoPlayerController const* const GeoController = Cast<AGeoPlayerController>(Controller);
	if (GeoController && InGoal != EGeoMenuGuideGoal::None && InGoal != EGeoMenuGuideGoal::Hold)
	{
		UGeoMenuRootWidget const* const Menu =
			GeoController->IsPauseMenuOpen() ? Cast<UGeoMenuRootWidget>(GeoController->GetPauseMenuWidget()) : nullptr;
		if (!Menu)
		{
			FKey const Key =
				UHudFunctionLibrary::GetDefaultMappedKey(GeoController->InputMapping, GeoController->ToggleMenuAction);
			Target.Hint = FText::Format(OpenMenuFormat, Key.GetDisplayName());
		}
		else if (UGeoMenuPageWidget const* const Page = Menu->GetShownPage())
		{
			Target = Page->GetGuideTarget(InGoal);
		}
		else
		{
			Target = Menu->GetGuideTarget(InGoal);
		}
	}

	return Target;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMenuGuideWidget::Refresh()
{
	// A page coming on top of the menu under a goal gets to set up where the path starts, once.
	AGeoPlayerController const* const GeoController = Cast<AGeoPlayerController>(GetOwningPlayer());
	UGeoMenuRootWidget* const Menu = GeoController && GeoController->IsPauseMenuOpen()
			&& Goal != EGeoMenuGuideGoal::None && Goal != EGeoMenuGuideGoal::Hold
		? Cast<UGeoMenuRootWidget>(GeoController->GetPauseMenuWidget())
		: nullptr;
	UGeoMenuPageWidget* const ShownPage = Menu ? Menu->GetShownPage() : nullptr;
	if (ShownPage != PreparedPage.Get() || Goal != PreparedGoal)
	{
		PreparedPage = ShownPage;
		PreparedGoal = Goal;
		if (ShownPage)
		{
			ShownPage->PrepareForGuide(Goal);
		}
	}

	FGeoMenuGuideTarget const Target = FindTarget(GetOwningPlayer(), Goal);
	TOptional<FSlateRect> Hole;
	for (UWidget const* const Lit : {Target.Widget.Get(), Target.AlsoLit.Get()})
	{
		if (Lit && Root.IsValid())
		{
			FGeometry const& RootGeometry = Root->GetCachedGeometry();
			FGeometry const& Geometry = Lit->GetCachedGeometry();
			FVector2f const Min = RootGeometry.AbsoluteToLocal(Geometry.GetAbsolutePosition());
			FVector2f const Max =
				RootGeometry.AbsoluteToLocal(Geometry.GetAbsolutePosition() + Geometry.GetAbsoluteSize());
			if (Max.X > Min.X && Max.Y > Min.Y)
			{
				FSlateRect const Rect(Min.X, Min.Y, Max.X, Max.Y);
				Hole = Hole.IsSet()
					? FSlateRect(FMath::Min(Hole->Left, Rect.Left), FMath::Min(Hole->Top, Rect.Top),
								 FMath::Max(Hole->Right, Rect.Right), FMath::Max(Hole->Bottom, Rect.Bottom))
					: Rect;
			}
		}
	}

	bool bHoleMoved = Hole.IsSet() != ShownHole.IsSet();
	if (Hole.IsSet() && ShownHole.IsSet())
	{
		FSlateRect const Now = Hole.GetValue();
		FSlateRect const Before = ShownHole.GetValue();
		bHoleMoved = !FMath::IsNearlyEqual(Now.Left, Before.Left, .5f)
			|| !FMath::IsNearlyEqual(Now.Top, Before.Top, .5f) || !FMath::IsNearlyEqual(Now.Right, Before.Right, .5f)
			|| !FMath::IsNearlyEqual(Now.Bottom, Before.Bottom, .5f);
	}

	if (Root.IsValid()
		&& (!bShown || ShownGoal != Goal || bHoleMoved || !Target.Hint.EqualTo(ShownHint)
			|| !Root->GetCachedGeometry().GetLocalSize().Equals(ShownCanvasSize, .5f)))
	{
		Rebuild(Hole, Target.Hint);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
FReply UGeoMenuGuideWidget::SwallowClick(FGeometry const& /*Geometry*/, FPointerEvent const& /*PointerEvent*/) const
{
	return FReply::Handled();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMenuGuideWidget::Rebuild(TOptional<FSlateRect> const& Hole, FText const& Hint)
{
	bShown = true;
	ShownGoal = Goal;
	ShownHole = Hole;
	ShownHint = Hint;
	TOptional<FSlateRect> Outline;
	if (Hole.IsSet())
	{
		Outline = Hole.GetValue().ExtendBy(FMargin(HolePadding));
	}

	if (Canvas.IsValid() && Root.IsValid())
	{
		Canvas->ClearChildren();
		ShownCanvasSize = Root->GetCachedGeometry().GetLocalSize();
		FSlateRect const Whole(FVector2f::ZeroVector, ShownCanvasSize);
		// Hold: no hole, the whole screen is one refusing part. Any other goal without a hole leaves it free.
		TArray<FSlateRect> Scrim;
		if (Hole.IsSet())
		{
			Scrim = GeoMenuGuide::MakeScrim(Whole, Hole.GetValue());
		}
		else if (Goal == EGeoMenuGuideGoal::Hold)
		{
			Scrim.Add(Whole);
		}
		{
			for (FSlateRect const& Part : Scrim)
			{
				// clang-format off
				Canvas->AddSlot()
					.Offset(FMargin(Part.Left, Part.Top, Part.GetSize().X, Part.GetSize().Y))
					.Alignment(FVector2D::ZeroVector)
					[
						SNew(SBorder)
						.BorderImage(&WhiteBrush)
						.BorderBackgroundColor(ScrimColor)
						.OnMouseButtonDown_UObject(this, &UGeoMenuGuideWidget::SwallowClick)
						.OnMouseButtonUp_UObject(this, &UGeoMenuGuideWidget::SwallowClick)
						.OnMouseDoubleClick_UObject(this, &UGeoMenuGuideWidget::SwallowClick)
					];
				// clang-format on
			}
		}

		UGeoUITheme const* const Theme = UGeoUITheme::Get();
		FGeoTextStyle const* const TextStyle = Theme ? Theme->FindTextStyle(HintTextRole) : nullptr;
		if (TextStyle && !Hint.IsEmpty())
		{
			FGeoMenuGuideHintPlace const Place = Hole.IsSet()
				? GeoMenuGuide::PlaceHint(Whole, Outline.GetValue(), HintRoom, HintGap)
				: FGeoMenuGuideHintPlace{Whole.GetSize() * .5f, FVector2f(.5f, .5f)};
			// clang-format off
			Canvas->AddSlot()
				.Offset(FMargin(Place.Position.X, Place.Position.Y, 0.f, 0.f))
				.Alignment(FVector2D(Place.Alignment))
				.AutoSize(true)
				[
					SNew(SBorder)
					.Visibility(EVisibility::HitTestInvisible)
					.BorderImage(&WhiteBrush)
					.BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, .85f))
					.Padding(HintPadding)
					[
						SNew(STextBlock)
						.Text(Hint)
						.Font(TextStyle->Font)
						.ColorAndOpacity(FrameColor)
					]
				];
			// clang-format on
		}
	}

	if (Frame.IsValid())
	{
		Frame->Set(Outline, FrameColor, FrameThickness, PulseSeconds, PulseReach);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMenuGuideWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

// ---------------------------------------------------------------------------------------------------------------------
TSharedRef<SWidget> UGeoMenuGuideWidget::RebuildWidget()
{
	bShown = false;
	// clang-format off
	SAssignNew(Root, SOverlay)
		.Visibility(EVisibility::SelfHitTestInvisible)
		+ SOverlay::Slot()
		[
			SAssignNew(Canvas, SConstraintCanvas)
			.Visibility(EVisibility::SelfHitTestInvisible)
		]
		+ SOverlay::Slot()
		[
			SAssignNew(Frame, SGeoGuideFrame)
			.Owner(this)
		];
	// clang-format on
	return Root.ToSharedRef();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMenuGuideWidget::ReleaseSlateResources(bool const bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	Frame.Reset();
	Canvas.Reset();
	Root.Reset();
}
