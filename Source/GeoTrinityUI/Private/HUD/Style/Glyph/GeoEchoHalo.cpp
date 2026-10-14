// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Style/Glyph/GeoEchoHalo.h"

#include "HUD/Style/Frame/GeoFrameStyle.h"
#include "HUD/Style/Glyph/GeoShapePainter.h"
#include "Widgets/SLeafWidget.h"

/** Leaf widget painting the halo of its UGeoEchoHalo, turning and running its squares on an active timer. */
class SGeoEchoHalo : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SGeoEchoHalo) {}
	SLATE_ARGUMENT(TWeakObjectPtr<UGeoEchoHalo>, Owner)
	SLATE_END_ARGS()

	void Construct(FArguments const& InArgs)
	{
		Owner = InArgs._Owner;
		RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SGeoEchoHalo::Animate));
	}

	virtual FVector2D ComputeDesiredSize(float /*LayoutScaleMultiplier*/) const override
	{
		return FVector2D::ZeroVector;
	}

	virtual int32 OnPaint(FPaintArgs const& /*Args*/, FGeometry const& AllottedGeometry,
						  FSlateRect const& /*MyCullingRect*/, FSlateWindowElementList& OutDrawElements, int32 LayerId,
						  FWidgetStyle const& InWidgetStyle, bool /*bParentEnabled*/) const override
	{
		UGeoEchoHalo const* Halo = Owner.Get();
		UGeoFrameStyle const* Style = Halo ? Halo->FrameStyle.Get() : nullptr;
		if (Halo && Style)
		{
			FVector2f const LocalSize = AllottedGeometry.GetLocalSize();
			FVector2f const Center = .5f * LocalSize;
			float const Half = .5f * FMath::Min(LocalSize.X, LocalSize.Y);
			float const Opacity = InWidgetStyle.GetColorAndOpacityTint().A;
			float const Activation = Halo->bLocked ? 0.f : State.Activation;
			FLinearColor const Tint = Halo->bLocked ? Halo->LockedColor : Halo->Color;

			TArray<FVector2f> const Ring = FGeoShapePainter::MakePolygon(Center, Half * Halo->RingRatio, 0, 0.f);
			FLinearColor Disc = Halo->DiscColor;
			Disc.A *= Opacity;
			FGeoShapePainter::DrawFilled(OutDrawElements, LayerId, AllottedGeometry, Ring, Disc);

			// Bloom: nested translucent discs of the colour, brightest at the middle, as emitted light.
			float const BloomAlpha = Halo->BloomAlpha * FMath::Lerp(1.f, Halo->ActiveBloomScale, Activation) * Opacity;
			for (int32 Layer = 1; Layer <= Halo->BloomLayers; ++Layer)
			{
				FLinearColor Bloom = Tint;
				Bloom.A *= BloomAlpha / Halo->BloomLayers;
				FGeoShapePainter::DrawFilled(
					OutDrawElements, LayerId, AllottedGeometry,
					FGeoShapePainter::MakePolygon(
						Center, Half * Halo->RingRatio * (1.f - (Layer - 1.f) / Halo->BloomLayers), 0, 0.f),
					Bloom);
			}

			FLinearColor GlowColor = Tint;
			GlowColor.A *= FMath::Lerp(Halo->IdleGlowAlpha, Halo->ActiveGlowAlpha, Activation) * Opacity;
			FLinearColor SquareColor = FMath::Lerp(Style->LineColor, Style->ActiveLineColor, Activation) * Tint;
			SquareColor.A *= Opacity;
			auto const DrawRosace = [&](float const Radius, float const Turning)
			{
				for (int32 Square = 0; Square < Halo->SquareCount; ++Square)
				{
					FGeoShapePainter::DrawOutline(
						OutDrawElements, LayerId + 1, AllottedGeometry,
						FGeoShapePainter::MakePolygon(Center, Radius, 4, Turning + Square * 90.f / Halo->SquareCount),
						SquareColor, Style->LineThickness);
				}
			};
			DrawRosace(Half * Halo->OuterRosaceRatio, Turn);
			DrawRosace(Half * Halo->InnerRosaceRatio, -Turn);

			FGeoShapePainter::DrawGlow(OutDrawElements, LayerId + 2, AllottedGeometry, Ring, GlowColor,
									   Style->GlowThickness * Halo->RingGlowScale);
			if (Halo->bLocked)
			{
				FGeoShapePainter::DrawOutline(OutDrawElements, LayerId + 3, AllottedGeometry, Ring, SquareColor,
											  Style->LineThickness);
			}
			else
			{
				FGeoShapePainter::PaintStyledLine(*Style, State, Opacity, Ring, AllottedGeometry, OutDrawElements,
												  LayerId + 2, FLinearColor::Transparent, Tint);
			}
		}
		return LayerId + 4;
	}

private:
	EActiveTimerReturnType Animate(double /*CurrentTime*/, float const DeltaTime)
	{
		UGeoEchoHalo* Halo = Owner.Get();
		UGeoFrameStyle const* Style = Halo ? Halo->FrameStyle.Get() : nullptr;
		if (Halo && Style && !Halo->bLocked)
		{
			if (Halo->bFlareRequested)
			{
				FlareLeft = Halo->FlareSeconds;
				Halo->bFlareRequested = false;
			}

			FlareLeft = FMath::Max(FlareLeft - DeltaTime, 0.f);
			FGeoShapePainter::AdvanceStyledState(*Style, Halo->bActive || FlareLeft > 0.f, DeltaTime, State);
			float const TurnSpeed = FMath::Lerp(Halo->IdleTurnSpeed, Halo->ActiveTurnSpeed, State.Activation);
			Turn = FMath::Fmod(Turn + TurnSpeed * DeltaTime, 360.f);
			Invalidate(EInvalidateWidgetReason::Paint);
		}
		return EActiveTimerReturnType::Continue;
	}

	TWeakObjectPtr<UGeoEchoHalo> Owner;
	/** Angle the outer rosace has turned, the inner one turning back by as much. */
	float Turn = 0.f;
	/** Seconds left of the flare that holds the active look. */
	float FlareLeft = 0.f;
	/** Activation and runner travel of the ring, as a styled outline keeps them. */
	FGeoOutlineState State;
};

// ---------------------------------------------------------------------------------------------------------------------
void UGeoEchoHalo::SetColor(FLinearColor const& InColor)
{
	Color = InColor;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoEchoHalo::SetLocked(bool const bInLocked)
{
	bLocked = bInLocked;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoEchoHalo::SetActive(bool const bInActive)
{
	bActive = bInActive;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoEchoHalo::PlayFlare()
{
	bFlareRequested = true;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoEchoHalo::ReleaseSlateResources(bool const bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyHalo.Reset();
}

#if WITH_EDITOR
// ---------------------------------------------------------------------------------------------------------------------
FText const UGeoEchoHalo::GetPaletteCategory()
{
	return INVTEXT("Geo");
}
#endif

// ---------------------------------------------------------------------------------------------------------------------
TSharedRef<SWidget> UGeoEchoHalo::RebuildWidget()
{
	ensureMsgf(FrameStyle, TEXT("%hs: FrameStyle is not set on %s"), __FUNCTION__, *GetName());
	MyHalo = SNew(SGeoEchoHalo).Owner(this);
	return MyHalo.ToSharedRef();
}
