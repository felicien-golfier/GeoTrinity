// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Style/GeoGemGlyph.h"

#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "HUD/Style/GeoShapePainter.h"
#include "HUD/Style/GeoUITheme.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Widgets/SLeafWidget.h"

/** Leaf widget painting one gem or socket over its whole box. */
class SGeoGemGlyph : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SGeoGemGlyph) {}
	SLATE_END_ARGS()

	void Construct(FArguments const& /*InArgs*/) {}

	void SetGlyph(EGeoGemTier const InTier, EGeoGemGlyphState const InState, FLinearColor const& InColor,
				  int32 const InUnlockLevel, bool const bInHighlighted, float const InSize)
	{
		Tier = InTier;
		State = InState;
		Color = InColor;
		UnlockLevel = InUnlockLevel;
		bHighlighted = bInHighlighted;
		Size = InSize;
		Invalidate(EInvalidateWidgetReason::Layout);
	}

	virtual FVector2D ComputeDesiredSize(float /*LayoutScaleMultiplier*/) const override
	{
		return FVector2D(Size, Size);
	}

	virtual int32 OnPaint(FPaintArgs const& /*Args*/, FGeometry const& AllottedGeometry,
						  FSlateRect const& /*MyCullingRect*/, FSlateWindowElementList& OutDrawElements, int32 LayerId,
						  FWidgetStyle const& InWidgetStyle, bool /*bParentEnabled*/) const override
	{
		UGeoUITheme const* Theme = UGeoUITheme::Get();
		if (!Theme)
		{
			return LayerId;
		}

		FGeoGemStyle const& Style = Theme->GemStyle;
		FGeoGemTierShape const Shape = Style.TierShapes.FindRef(Tier);
		FVector2f const LocalSize = AllottedGeometry.GetLocalSize();
		FVector2f const Center = .5f * LocalSize;
		float const Radius = .5f * FMath::Min(LocalSize.X, LocalSize.Y) - Style.LineThickness;
		FLinearColor const Tint = InWidgetStyle.GetColorAndOpacityTint();
		auto Polygon = [&Center, &Shape](float const PolygonRadius)
		{
			return FGeoShapePainter::MakePolygon(Center, PolygonRadius, Shape.Sides, Shape.Rotation);
		};
		TArray<FVector2f> const Outline = Polygon(Radius);

		if (bHighlighted)
		{
			FGeoShapePainter::DrawGlow(OutDrawElements, LayerId, AllottedGeometry, Outline, Style.HighlightColor * Tint,
									   4.f * Style.LineThickness);
		}

		if (State == EGeoGemGlyphState::Gem)
		{
			FGeoShapePainter::DrawFilled(OutDrawElements, LayerId + 1, AllottedGeometry, Outline, Color * Tint);
			if (Tier == EGeoGemTier::Core)
			{
				FGeoShapePainter::DrawFilled(OutDrawElements, LayerId + 1, AllottedGeometry,
											 Polygon(Radius * Style.CoreRingScale), Style.CoreRingColor * Tint);
				FGeoShapePainter::DrawFilled(OutDrawElements, LayerId + 1, AllottedGeometry,
											 Polygon(Radius * Style.CoreCentreScale), Style.CoreCentreColor * Tint);
			}
			else
			{
				FGeoShapePainter::DrawFilled(OutDrawElements, LayerId + 1, AllottedGeometry,
											 Polygon(Radius * Style.FacetScale), Style.FacetColor * Tint);
			}
		}
		else
		{
			bool const bLocked = State == EGeoGemGlyphState::LockedSocket;
			FLinearColor const LineColor = bHighlighted ? Style.HighlightColor
										 : bLocked		? Style.LockedLineColor
														: Style.SocketLineColor;
			FGeoShapePainter::DrawFilled(OutDrawElements, LayerId + 1, AllottedGeometry, Outline,
										 Style.SocketFillColor * Tint);
			FGeoShapePainter::DrawOutline(OutDrawElements, LayerId + 1, AllottedGeometry, Outline, LineColor * Tint,
										  Style.LineThickness);
			if (bLocked)
			{
				PaintUnlockLevel(*Theme, AllottedGeometry, OutDrawElements, LayerId + 2, Tint);
			}
		}

		if (bHighlighted)
		{
			FGeoShapePainter::DrawOutline(OutDrawElements, LayerId + 2, AllottedGeometry, Outline,
										  Style.HighlightColor * Tint, Style.LineThickness);
		}
		return LayerId + 2;
	}

private:
	/** The unlock level, centred, in the theme's Mono font scaled to the socket. */
	void PaintUnlockLevel(UGeoUITheme const& Theme, FGeometry const& AllottedGeometry,
						  FSlateWindowElementList& OutDrawElements, int32 const LayerId, FLinearColor const& Tint) const
	{
		FGeoTextStyle const* TextStyle = Theme.FindTextStyle(EGeoTextRole::Mono);
		if (!TextStyle)
		{
			return;
		}

		FSlateFontInfo Font = TextStyle->Font;
		Font.Size = FMath::Max(6.f, Theme.GemStyle.LockTextScale * Size);
		FString const Level = FString::FromInt(UnlockLevel);
		FVector2f const TextSize(
			FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Level, Font));
		FVector2f const Offset = .5f * (AllottedGeometry.GetLocalSize() - TextSize);
		FSlateDrawElement::MakeText(OutDrawElements, LayerId,
									AllottedGeometry.ToPaintGeometry(TextSize, FSlateLayoutTransform(Offset)), Level,
									Font, ESlateDrawEffect::None, Theme.GemStyle.LockTextColor * Tint);
	}

	EGeoGemTier Tier = EGeoGemTier::Chip;
	EGeoGemGlyphState State = EGeoGemGlyphState::Gem;
	FLinearColor Color = FLinearColor::White;
	int32 UnlockLevel = 1;
	bool bHighlighted = false;
	float Size = 40.f;
};

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemGlyph::SetGem(EGeoGemTier const InTier, FLinearColor const& InColor)
{
	Tier = InTier;
	Color = InColor;
	State = EGeoGemGlyphState::Gem;
	SynchronizeProperties();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemGlyph::SetSocket(EGeoGemTier const InTier, bool const bLocked, int32 const InUnlockLevel)
{
	Tier = InTier;
	State = bLocked ? EGeoGemGlyphState::LockedSocket : EGeoGemGlyphState::EmptySocket;
	UnlockLevel = InUnlockLevel;
	SynchronizeProperties();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemGlyph::SetHighlighted(bool const bInHighlighted)
{
	bHighlighted = bInHighlighted;
	SynchronizeProperties();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemGlyph::SetSize(float const InSize)
{
	Size = InSize;
	SynchronizeProperties();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemGlyph::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (MyGlyph)
	{
		MyGlyph->SetGlyph(Tier, State, Color, UnlockLevel, bHighlighted, Size);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemGlyph::ReleaseSlateResources(bool const bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyGlyph.Reset();
}

#if WITH_EDITOR
// ---------------------------------------------------------------------------------------------------------------------
FText const UGeoGemGlyph::GetPaletteCategory()
{
	return INVTEXT("Geo");
}
#endif

// ---------------------------------------------------------------------------------------------------------------------
TSharedRef<SWidget> UGeoGemGlyph::RebuildWidget()
{
	MyGlyph = SNew(SGeoGemGlyph);
	return MyGlyph.ToSharedRef();
}
