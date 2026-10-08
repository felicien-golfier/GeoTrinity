// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Style/GeoShape.h"

#include "HUD/Style/GeoFrameStyle.h"
#include "HUD/Style/GeoShapePainter.h"
#include "HUD/Style/GeoUITheme.h"
#include "Widgets/SLeafWidget.h"

/** Leaf widget drawing one regular polygon, turning it on an active timer while it has a spin speed. */
class SGeoShape : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SGeoShape) {}
	SLATE_END_ARGS()

	void Construct(FArguments const& /*InArgs*/)
	{
		RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SGeoShape::Animate));
	}

	void SetShape(int32 const InSides, float const InSize, float const InRotation, float const InSpinSpeed,
				  bool const bInFilled, float const InLineThickness, FLinearColor const& InColor,
				  UGeoFrameStyle const* InOutlineStyle)
	{
		OutlineStyle = InOutlineStyle;
		Sides = InSides;
		Size = InSize;
		Rotation = InRotation;
		SpinSpeed = InSpinSpeed;
		bFilled = bInFilled;
		LineThickness = InLineThickness;
		Color = InColor;
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
		UGeoFrameStyle const* Style = OutlineStyle.Get();
		FVector2f const BoxSize = AllottedGeometry.GetLocalSize();
		float const Inset = Style ? .5f * Style->LineThickness : bFilled ? 0.f : .5f * LineThickness;
		TArray<FVector2f> const Points = FGeoShapePainter::MakePolygon(
			.5f * BoxSize, .5f * FMath::Min(BoxSize.X, BoxSize.Y) - Inset, Sides, Rotation + Spin);

		float const Opacity = InWidgetStyle.GetColorAndOpacityTint().A;
		FLinearColor ShapeColor = Color;
		ShapeColor.A *= Opacity;
		if (Style)
		{
			FGeoShapePainter::PaintStyledFill(*Style, State, Opacity, Points, AllottedGeometry, OutDrawElements,
											  LayerId);
			FGeoShapePainter::PaintStyledLine(*Style, State, Opacity, Points, AllottedGeometry, OutDrawElements,
											  LayerId + 1);
		}
		else if (bFilled)
		{
			FGeoShapePainter::DrawFilled(OutDrawElements, LayerId, AllottedGeometry, Points, ShapeColor);
		}
		else
		{
			FGeoShapePainter::DrawOutline(OutDrawElements, LayerId, AllottedGeometry, Points, ShapeColor,
										  LineThickness);
		}
		// A styled outline takes a layer for its fill, then three for its glow, line and runners.
		return Style ? LayerId + 4 : LayerId;
	}

private:
	EActiveTimerReturnType Animate(double /*CurrentTime*/, float const DeltaTime)
	{
		if (UGeoFrameStyle const* Style = OutlineStyle.Get())
		{
			FGeoShapePainter::AdvanceStyledState(*Style, false, DeltaTime, State);
			Invalidate(EInvalidateWidgetReason::Paint);
		}
		if (SpinSpeed != 0.f)
		{
			Spin = FMath::Fmod(Spin + SpinSpeed * DeltaTime, 360.f);
			Invalidate(EInvalidateWidgetReason::Paint);
		}
		return EActiveTimerReturnType::Continue;
	}

	int32 Sides = 6;
	float Size = 64.f;
	float Rotation = 0.f;
	float SpinSpeed = 0.f;
	bool bFilled = false;
	float LineThickness = 1.5f;
	FLinearColor Color = FLinearColor::White;
	TWeakObjectPtr<UGeoFrameStyle const> OutlineStyle;
	/** Angle turned so far by SpinSpeed, on top of Rotation. */
	float Spin = 0.f;
	/** Runner travel of OutlineStyle; a shape is never active. */
	FGeoOutlineState State;
};

// ---------------------------------------------------------------------------------------------------------------------
void UGeoShape::SetClassShape(FGeoClassStyle const& ClassStyle)
{
	Sides = ClassStyle.Sides;
	Rotation = ClassStyle.Rotation;
	Color = ClassStyle.Color;
	bFilled = true;
	SynchronizeProperties();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoShape::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (MyShape)
	{
		MyShape->SetShape(Sides, Size, Rotation, SpinSpeed, bFilled, LineThickness, Color, OutlineStyle);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoShape::ReleaseSlateResources(bool const bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyShape.Reset();
}

#if WITH_EDITOR
// ---------------------------------------------------------------------------------------------------------------------
FText const UGeoShape::GetPaletteCategory()
{
	return INVTEXT("Geo");
}
#endif

// ---------------------------------------------------------------------------------------------------------------------
TSharedRef<SWidget> UGeoShape::RebuildWidget()
{
	MyShape = SNew(SGeoShape);
	return MyShape.ToSharedRef();
}
