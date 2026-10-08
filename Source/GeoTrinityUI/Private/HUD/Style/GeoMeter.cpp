// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Style/GeoMeter.h"

#include "HUD/Style/GeoMeterStyle.h"
#include "HUD/Style/GeoShapePainter.h"
#include "Widgets/SLeafWidget.h"

static int32 const ArcSegmentsPerTurn = 64;

/** Leaf widget painting one meter in its style. */
class SGeoMeter : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SGeoMeter) {}
	SLATE_END_ARGS()

	void Construct(FArguments const& /*InArgs*/) {}

	void SetMeter(UGeoMeterStyle const* InStyle, float const InFill, float const InOverhang,
				  FLinearColor const& InFillTint, int32 const InRingSides, float const InRingRotation,
				  bool const bInReady)
	{
		bool const bResized = Style.Get() != InStyle;
		Style = InStyle;
		Fill = FMath::Clamp(InFill, 0.f, 1.f);
		Overhang = FMath::Clamp(InOverhang, 0.f, 1.f);
		FillTint = InFillTint;
		RingSides = InRingSides;
		RingRotation = InRingRotation;
		if (bInReady && !bReady)
		{
			ReadyTime = 0.f;
			RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SGeoMeter::AnimateReady));
		}
		bReady = bInReady;
		Invalidate(bResized ? EInvalidateWidgetReason::Layout : EInvalidateWidgetReason::Paint);
	}

	virtual FVector2D ComputeDesiredSize(float /*LayoutScaleMultiplier*/) const override
	{
		UGeoMeterStyle const* MeterStyle = Style.Get();
		return MeterStyle ? MeterStyle->Size : FVector2D(200.f, 8.f);
	}

	virtual int32 OnPaint(FPaintArgs const& /*Args*/, FGeometry const& AllottedGeometry,
						  FSlateRect const& /*MyCullingRect*/, FSlateWindowElementList& OutDrawElements, int32 LayerId,
						  FWidgetStyle const& InWidgetStyle, bool /*bParentEnabled*/) const override
	{
		UGeoMeterStyle const* MeterStyle = Style.Get();
		if (!MeterStyle)
		{
			return LayerId;
		}

		float const Opacity = InWidgetStyle.GetColorAndOpacityTint().A;
		switch (MeterStyle->Shape)
		{
		case EGeoMeterShape::Bar:
			PaintBar(*MeterStyle, Opacity, AllottedGeometry, OutDrawElements, LayerId);
			return LayerId + 4;
		case EGeoMeterShape::Ring:
			PaintRing(*MeterStyle, Opacity, AllottedGeometry, OutDrawElements, LayerId);
			return LayerId + 2;
		case EGeoMeterShape::Sweep:
			PaintSweep(*MeterStyle, Opacity, AllottedGeometry, OutDrawElements, LayerId);
			return LayerId;
		}
		return LayerId;
	}

private:
	EActiveTimerReturnType AnimateReady(double /*CurrentTime*/, float const DeltaTime)
	{
		ReadyTime += DeltaTime;
		Invalidate(EInvalidateWidgetReason::Paint);
		return bReady ? EActiveTimerReturnType::Continue : EActiveTimerReturnType::Stop;
	}

	static FLinearColor Faded(FLinearColor Color, float const Opacity)
	{
		Color.A *= Opacity;
		return Color;
	}

	void PaintBar(UGeoMeterStyle const& MeterStyle, float const Opacity, FGeometry const& Geometry,
				  FSlateWindowElementList& OutDrawElements, int32 const LayerId) const
	{
		FVector2f const Size = Geometry.GetLocalSize();
		FLinearColor const Track = Fill < MeterStyle.LowThreshold ? MeterStyle.LowTrackColor : MeterStyle.TrackColor;
		FGeoShapePainter::DrawFilled(OutDrawElements, LayerId, Geometry,
									 FGeoShapePainter::MakeRectangle(FVector2f::ZeroVector, Size),
									 Faded(Track, Opacity));

		if (Overhang > 0.f)
		{
			float const Reach = MeterStyle.OverhangSize;
			TArray<FVector2f> const OverhangBox =
				FGeoShapePainter::MakeRectangle(FVector2f(-Reach), FVector2f(Overhang * Size.X, Size.Y + Reach));
			FGeoShapePainter::DrawFilled(OutDrawElements, LayerId + 1, Geometry, OverhangBox,
										 Faded(MeterStyle.OverhangFillColor, Opacity));
			FGeoShapePainter::DrawOutline(OutDrawElements, LayerId + 1, Geometry, OverhangBox,
										  Faded(MeterStyle.OverhangLineColor, Opacity),
										  MeterStyle.OverhangLineThickness);
		}

		if (Fill > 0.f)
		{
			FLinearColor const FillColor = MeterStyle.FillColor * FillTint;
			TArray<FVector2f> const FillBox =
				FGeoShapePainter::MakeRectangle(FVector2f::ZeroVector, FVector2f(Fill * Size.X, Size.Y));
			FGeoShapePainter::DrawGlow(OutDrawElements, LayerId + 2, Geometry, FillBox,
									   Faded(FillColor, MeterStyle.GlowOpacity * Opacity), MeterStyle.GlowThickness);
			FGeoShapePainter::DrawFilled(OutDrawElements, LayerId + 2, Geometry, FillBox, Faded(FillColor, Opacity));
		}

		for (int32 Tick = 1; Tick < MeterStyle.TickParts; ++Tick)
		{
			float const X = Size.X * Tick / MeterStyle.TickParts;
			float const HalfWidth = .5f * MeterStyle.TickWidth;
			FGeoShapePainter::DrawFilled(
				OutDrawElements, LayerId + 3, Geometry,
				FGeoShapePainter::MakeRectangle(FVector2f(X - HalfWidth, -MeterStyle.TickOverhang),
												FVector2f(X + HalfWidth, Size.Y + MeterStyle.TickOverhang)),
				Faded(MeterStyle.TickColor, Opacity));
		}

		if (MeterStyle.OutlineThickness > 0.f)
		{
			TArray<FVector2f> const Outline =
				FGeoShapePainter::MakeCutRectangle(Size, .5f * MeterStyle.OutlineThickness, 0.f);
			FGeoShapePainter::DrawGlow(OutDrawElements, LayerId + 4, Geometry, Outline,
									   Faded(MeterStyle.OutlineGlowColor * FillTint, Opacity),
									   MeterStyle.OutlineGlowThickness);
			FGeoShapePainter::DrawOutline(OutDrawElements, LayerId + 4, Geometry, Outline,
										  Faded(MeterStyle.OutlineColor, Opacity), MeterStyle.OutlineThickness);
		}
	}

	void PaintRing(UGeoMeterStyle const& MeterStyle, float const Opacity, FGeometry const& Geometry,
				   FSlateWindowElementList& OutDrawElements, int32 const LayerId) const
	{
		FVector2f const Size = Geometry.GetLocalSize();
		float const Pulse = bReady ? .5f - .5f * FMath::Cos(UE_TWO_PI * ReadyTime / MeterStyle.ReadyPulseSeconds) : 0.f;
		float const Radius =
			.5f * (FMath::Min(Size.X, Size.Y) - MeterStyle.RingThickness) * (1.f + MeterStyle.ReadyPulseScale * Pulse);
		TArray<FVector2f> const Outline = FGeoShapePainter::MakePolygon(.5f * Size, Radius, RingSides, RingRotation);
		FGeoShapePainter::DrawOutline(OutDrawElements, LayerId, Geometry, Outline,
									  Faded(MeterStyle.TrackColor, Opacity), MeterStyle.RingThickness);

		FLinearColor const FillColor = Faded(MeterStyle.FillColor * FillTint, Opacity);
		if (bReady)
		{
			FGeoShapePainter::DrawGlow(OutDrawElements, LayerId + 1, Geometry, Outline,
									   Faded(MeterStyle.ReadyGlowColor * FillTint, Pulse * Opacity),
									   MeterStyle.ReadyGlowThickness);
		}

		if (Fill >= 1.f)
		{
			FGeoShapePainter::DrawOutline(OutDrawElements, LayerId + 1, Geometry, Outline, FillColor,
										  MeterStyle.RingThickness);
		}
		else if (Fill > 0.f)
		{
			TArray<FVector2f> const Part =
				FGeoShapePainter::GetOutlinePart(Outline, Fill * FGeoShapePainter::GetOutlineLength(Outline));
			FGeoShapePainter::DrawOutline(OutDrawElements, LayerId + 1, Geometry, Part, FillColor,
										  MeterStyle.RingThickness, false);
		}

		float const Length = FGeoShapePainter::GetOutlineLength(Outline);
		for (int32 Orbiter = 0; bReady && Orbiter < MeterStyle.ReadyOrbiterCount; ++Orbiter)
		{
			float const Turns =
				ReadyTime / MeterStyle.ReadyOrbitSeconds + static_cast<float>(Orbiter) / MeterStyle.ReadyOrbiterCount;
			FVector2f const Position = FGeoShapePainter::GetPointAlongOutline(Outline, Turns * Length);
			FGeoShapePainter::DrawFilled(
				OutDrawElements, LayerId + 2, Geometry,
				FGeoShapePainter::MakePolygon(Position, .5f * MeterStyle.ReadyOrbiterSize, RingSides,
											  RingRotation + MeterStyle.ReadyOrbiterSpin * ReadyTime),
				Faded(MeterStyle.ReadyOrbiterColor, Opacity));
		}
	}

	void PaintSweep(UGeoMeterStyle const& MeterStyle, float const Opacity, FGeometry const& Geometry,
					FSlateWindowElementList& OutDrawElements, int32 const LayerId) const
	{
		if (Fill <= 0.f)
		{
			return;
		}

		// A fan from the centre to the box's edge, covering the last Fill of a turn clockwise from the top.
		FVector2f const HalfSize = .5f * Geometry.GetLocalSize();
		int32 const Segments = FMath::Max(2, FMath::CeilToInt32(Fill * ArcSegmentsPerTurn * 2));
		TArray<FVector2f> Pie;
		Pie.Reserve(Segments + 2);
		Pie.Add(HalfSize);
		for (int32 Segment = 0; Segment <= Segments; ++Segment)
		{
			float const Angle = FMath::DegreesToRadians(360.f * (1.f - Fill + Fill * Segment / Segments));
			FVector2f const Direction(FMath::Sin(Angle), -FMath::Cos(Angle));
			float const ToEdge = FMath::Min(HalfSize.X / FMath::Max(FMath::Abs(Direction.X), UE_KINDA_SMALL_NUMBER),
											HalfSize.Y / FMath::Max(FMath::Abs(Direction.Y), UE_KINDA_SMALL_NUMBER));
			Pie.Add(HalfSize + ToEdge * Direction);
		}
		FGeoShapePainter::DrawFilled(OutDrawElements, LayerId, Geometry, Pie,
									 Faded(MeterStyle.FillColor * FillTint, Opacity));
	}

	TWeakObjectPtr<UGeoMeterStyle const> Style;
	float Fill = 1.f;
	float Overhang = 0.f;
	FLinearColor FillTint = FLinearColor::White;
	int32 RingSides = 0;
	float RingRotation = 0.f;
	bool bReady = false;
	/** Seconds since the meter became ready: the pulse's and the orbiters' clock. */
	float ReadyTime = 0.f;
};

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMeter::SetFill(float const InFill)
{
	if (Fill != InFill)
	{
		Fill = InFill;
		SynchronizeProperties();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMeter::SetOverhang(float const InOverhang)
{
	if (Overhang != InOverhang)
	{
		Overhang = InOverhang;
		SynchronizeProperties();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMeter::SetFillTint(FLinearColor const& InFillTint)
{
	if (FillTint != InFillTint)
	{
		FillTint = InFillTint;
		SynchronizeProperties();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMeter::SetMeterStyle(UGeoMeterStyle* InMeterStyle)
{
	MeterStyle = InMeterStyle;
	SynchronizeProperties();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMeter::SetRingShape(int32 const InSides, float const InRotation)
{
	if (RingSides != InSides || RingRotation != InRotation)
	{
		RingSides = InSides;
		RingRotation = InRotation;
		SynchronizeProperties();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMeter::SetReady(bool const bInReady)
{
	if (bReady != bInReady)
	{
		bReady = bInReady;
		SynchronizeProperties();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMeter::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (MyMeter)
	{
		MyMeter->SetMeter(MeterStyle, Fill, Overhang, FillTint, RingSides, RingRotation, bReady);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMeter::ReleaseSlateResources(bool const bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyMeter.Reset();
}

#if WITH_EDITOR
// ---------------------------------------------------------------------------------------------------------------------
FText const UGeoMeter::GetPaletteCategory()
{
	return INVTEXT("Geo");
}
#endif

// ---------------------------------------------------------------------------------------------------------------------
TSharedRef<SWidget> UGeoMeter::RebuildWidget()
{
	MyMeter = SNew(SGeoMeter);
	return MyMeter.ToSharedRef();
}
