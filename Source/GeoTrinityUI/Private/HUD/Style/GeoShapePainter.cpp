// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Style/GeoShapePainter.h"

#include "Framework/Application/SlateApplication.h"
#include "HUD/Style/GeoFrameStyle.h"
#include "Layout/Geometry.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"
#include "Tool/GeoIcon.h"

static int32 const CircleSegments = 48;
// A glow is this many nested strokes, each thinner and sharing the glow's alpha, so it fades away from the line.
static int32 const GlowPasses = 4;

// ---------------------------------------------------------------------------------------------------------------------
TArray<FVector2f> FGeoShapePainter::MakePolygon(FVector2f const Center, float const Radius, int32 const Sides,
												float const AngleDegrees)
{
	int32 const Corners = Sides < 3 ? CircleSegments : Sides;
	TArray<FVector2f> Points;
	Points.Reserve(Corners);
	for (int32 Corner = 0; Corner < Corners; ++Corner)
	{
		float const Angle = FMath::DegreesToRadians(AngleDegrees + 360.f * Corner / Corners);
		Points.Add(Center + Radius * FVector2f(FMath::Sin(Angle), -FMath::Cos(Angle)));
	}
	return Points;
}

// ---------------------------------------------------------------------------------------------------------------------
TArray<FVector2f> FGeoShapePainter::MakeRectangle(FVector2f const Min, FVector2f const Max)
{
	return {Min, {Max.X, Min.Y}, Max, {Min.X, Max.Y}};
}

// ---------------------------------------------------------------------------------------------------------------------
TArray<FVector2f> FGeoShapePainter::MakeCutRectangle(FVector2f const Size, float const Inset, float const CornerCut)
{
	float const Left = Inset;
	float const Top = Inset;
	float const Right = Size.X - Inset;
	float const Bottom = Size.Y - Inset;
	float const Cut = FMath::Min(CornerCut, .5f * FMath::Min(Right - Left, Bottom - Top));
	if (Cut <= 0.f)
	{
		return {{Left, Top}, {Right, Top}, {Right, Bottom}, {Left, Bottom}};
	}

	return {{Left + Cut, Top},	   {Right - Cut, Top},	  {Right, Top + Cut}, {Right, Bottom - Cut},
			{Right - Cut, Bottom}, {Left + Cut, Bottom}, {Left, Bottom - Cut}, {Left, Top + Cut}};
}

// ---------------------------------------------------------------------------------------------------------------------
float FGeoShapePainter::GetOutlineLength(TArray<FVector2f> const& Points)
{
	float Length = 0.f;
	for (int32 Index = 0; Index < Points.Num(); ++Index)
	{
		Length += FVector2f::Distance(Points[Index], Points[(Index + 1) % Points.Num()]);
	}
	return Length;
}

// ---------------------------------------------------------------------------------------------------------------------
FVector2f FGeoShapePainter::GetPointAlongOutline(TArray<FVector2f> const& Points, float const Distance)
{
	float const Length = GetOutlineLength(Points);
	float Remaining = Length > 0.f ? FMath::Fmod(Distance, Length) : 0.f;
	if (Remaining < 0.f)
	{
		Remaining += Length;
	}

	for (int32 Index = 0; Index < Points.Num(); ++Index)
	{
		FVector2f const Start = Points[Index];
		FVector2f const End = Points[(Index + 1) % Points.Num()];
		float const SegmentLength = FVector2f::Distance(Start, End);
		if (Remaining <= SegmentLength && SegmentLength > 0.f)
		{
			return FMath::Lerp(Start, End, Remaining / SegmentLength);
		}
		Remaining -= SegmentLength;
	}
	return Points.IsEmpty() ? FVector2f::ZeroVector : Points[0];
}

// ---------------------------------------------------------------------------------------------------------------------
TArray<FVector2f> FGeoShapePainter::GetOutlinePart(TArray<FVector2f> const& Points, float const Length)
{
	TArray<FVector2f> Part;
	float Remaining = Length;
	for (int32 Index = 0; Index < Points.Num() && Remaining > 0.f; ++Index)
	{
		FVector2f const Start = Points[Index];
		FVector2f const End = Points[(Index + 1) % Points.Num()];
		float const SegmentLength = FVector2f::Distance(Start, End);
		Part.Add(Start);
		if (Remaining <= SegmentLength && SegmentLength > 0.f)
		{
			Part.Add(FMath::Lerp(Start, End, Remaining / SegmentLength));
		}
		Remaining -= SegmentLength;
	}
	return Part;
}

// ---------------------------------------------------------------------------------------------------------------------
void FGeoShapePainter::DrawFilled(FSlateWindowElementList& OutDrawElements, int32 const LayerId,
								  FGeometry const& Geometry, TArray<FVector2f> const& Points, FLinearColor const& Color)
{
	if (Points.Num() < 3 || Color.A <= 0.f)
	{
		return;
	}

	FSlateRenderTransform const& RenderTransform = Geometry.GetAccumulatedRenderTransform();
	FColor const VertexColor = Color.ToFColor(true);

	// A fan from the first corner, which covers any convex polygon.
	TArray<FSlateVertex> Vertices;
	Vertices.Reserve(Points.Num());
	for (FVector2f const& Point : Points)
	{
		Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(RenderTransform, Point, FVector2f::ZeroVector,
																		 VertexColor));
	}
	TArray<SlateIndex> Indices;
	Indices.Reserve((Points.Num() - 2) * 3);
	for (int32 Corner = 1; Corner + 1 < Points.Num(); ++Corner)
	{
		Indices.Append({0, static_cast<SlateIndex>(Corner), static_cast<SlateIndex>(Corner + 1)});
	}
	FSlateDrawElement::MakeCustomVerts(OutDrawElements, LayerId, GetWhiteTexture(), Vertices, Indices, nullptr, 0, 0);

	// The fan's edge is aliased; a hairline of the same colour over it smooths it.
	DrawOutline(OutDrawElements, LayerId, Geometry, Points, Color, 1.f);
}

// ---------------------------------------------------------------------------------------------------------------------
void FGeoShapePainter::DrawOutline(FSlateWindowElementList& OutDrawElements, int32 const LayerId,
								   FGeometry const& Geometry, TArray<FVector2f> const& Points,
								   FLinearColor const& Color, float const Thickness, bool const bClosed)
{
	if (Points.Num() < 2 || Color.A <= 0.f || Thickness <= 0.f)
	{
		return;
	}

	// A closed band of triangles rather than an antialiased polyline, whose open ends would fade into a hole where they
	// meet. Across the band four rings: clear, solid, solid, clear; the clear rings sit one pixel out and feather it.
	float const Pixel = 1.f / FMath::Max(Geometry.Scale, UE_KINDA_SMALL_NUMBER);
	float const CoreHalf = .5f * FMath::Max(Thickness - Pixel, 0.f);
	float const RingOffsets[] = {-CoreHalf - Pixel, -CoreHalf, CoreHalf, CoreHalf + Pixel};
	int32 const RingCount = UE_ARRAY_COUNT(RingOffsets);

	FLinearColor Solid = Color;
	Solid.A *= FMath::Min(Thickness / Pixel, 1.f);
	FColor const SolidColor = Solid.ToFColor(true);
	FColor ClearColor = SolidColor;
	ClearColor.A = 0;

	FSlateRenderTransform const& RenderTransform = Geometry.GetAccumulatedRenderTransform();
	int32 const Corners = Points.Num();
	TArray<FSlateVertex> Vertices;
	Vertices.Reserve(Corners * RingCount);
	for (int32 Corner = 0; Corner < Corners; ++Corner)
	{
		// An open line's ends have one edge each, which then stands in for the missing one.
		bool const bOpenStart = !bClosed && Corner == 0;
		bool const bOpenEnd = !bClosed && Corner == Corners - 1;
		FVector2f const Next = bOpenEnd ? 2.f * Points[Corner] - Points[Corner - 1] : Points[(Corner + 1) % Corners];
		FVector2f const Previous =
			bOpenStart ? 2.f * Points[Corner] - Next : Points[(Corner + Corners - 1) % Corners];
		FVector2f const InNormal = (Points[Corner] - Previous).GetSafeNormal().GetRotated(90.f);
		FVector2f const OutNormal = (Next - Points[Corner]).GetSafeNormal().GetRotated(90.f);
		FVector2f const MiterDirection = (InNormal + OutNormal).GetSafeNormal();
		FVector2f const Miter =
			MiterDirection / FMath::Max(FVector2f::DotProduct(MiterDirection, OutNormal), .25f);
		for (int32 Ring = 0; Ring < RingCount; ++Ring)
		{
			bool const bEdgeRing = Ring == 0 || Ring == RingCount - 1;
			Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
				RenderTransform, Points[Corner] + RingOffsets[Ring] * Miter, FVector2f::ZeroVector,
				bEdgeRing ? ClearColor : SolidColor));
		}
	}

	TArray<SlateIndex> Indices;
	int32 const Segments = bClosed ? Corners : Corners - 1;
	Indices.Reserve(Segments * (RingCount - 1) * 6);
	for (int32 Corner = 0; Corner < Segments; ++Corner)
	{
		int32 const Start = Corner * RingCount;
		int32 const End = (Corner + 1) % Corners * RingCount;
		for (int32 Ring = 0; Ring + 1 < RingCount; ++Ring)
		{
			Indices.Append({static_cast<SlateIndex>(Start + Ring), static_cast<SlateIndex>(Start + Ring + 1),
							static_cast<SlateIndex>(End + Ring), static_cast<SlateIndex>(Start + Ring + 1),
							static_cast<SlateIndex>(End + Ring + 1), static_cast<SlateIndex>(End + Ring)});
		}
	}
	FSlateDrawElement::MakeCustomVerts(OutDrawElements, LayerId, GetWhiteTexture(), Vertices, Indices, nullptr, 0, 0);
}

// ---------------------------------------------------------------------------------------------------------------------
FSlateResourceHandle FGeoShapePainter::GetWhiteTexture()
{
	return FSlateApplication::Get().GetRenderer()->GetResourceHandle(*FCoreStyle::Get().GetBrush("WhiteBrush"));
}

// ---------------------------------------------------------------------------------------------------------------------
void FGeoShapePainter::DrawGlow(FSlateWindowElementList& OutDrawElements, int32 const LayerId,
								FGeometry const& Geometry, TArray<FVector2f> const& Points, FLinearColor const& Color,
								float const Thickness)
{
	FLinearColor PassColor = Color;
	PassColor.A /= GlowPasses;
	for (int32 Pass = 0; Pass < GlowPasses; ++Pass)
	{
		DrawOutline(OutDrawElements, LayerId, Geometry, Points, PassColor, Thickness * (GlowPasses - Pass) / GlowPasses);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void FGeoShapePainter::PaintStyledFill(UGeoFrameStyle const& Style, FGeoOutlineState const& State, float const Opacity,
									   TArray<FVector2f> const& Outline, FGeometry const& Geometry,
									   FSlateWindowElementList& OutDrawElements, int32 const LayerId)
{
	FLinearColor Fill = FMath::Lerp(Style.FillColor, Style.ActiveFillColor, State.Activation);
	Fill.A *= Opacity;
	DrawFilled(OutDrawElements, LayerId, Geometry, Outline, Fill);
}

// ---------------------------------------------------------------------------------------------------------------------
void FGeoShapePainter::PaintStyledLine(UGeoFrameStyle const& Style, FGeoOutlineState const& State, float const Opacity,
									   TArray<FVector2f> const& Outline, FGeometry const& Geometry,
									   FSlateWindowElementList& OutDrawElements, int32 const LayerId,
									   FLinearColor const& GlowTint, FLinearColor const& LineTint)
{
	auto const Blend = [&State, Opacity](FLinearColor const& Idle, FLinearColor const& Active)
	{
		FLinearColor Color = FMath::Lerp(Idle, Active, State.Activation);
		Color.A *= Opacity;
		return Color;
	};
	DrawGlow(OutDrawElements, LayerId, Geometry, Outline, Blend(Style.GlowColor, Style.ActiveGlowColor) * GlowTint,
			 Style.GlowThickness);
	DrawOutline(OutDrawElements, LayerId + 1, Geometry, Outline, Blend(Style.LineColor, Style.ActiveLineColor) * LineTint,
				Style.LineThickness);

	// Runner slots are shared by the idle and the active set; a slot fades in or out as the frame eases between them.
	auto const IsInEvenSet = [](int32 const Index, int32 const Count, int32 const Total)
	{
		return Count > 0 && (Index * Count) % Total < Count ? 1.f : 0.f;
	};
	int32 const Total = FMath::Max(Style.RunnerCount, Style.ActiveRunnerCount);
	double const Length = GetOutlineLength(Outline);
	for (int32 Runner = 0; Runner < Total && Length > 0.0; ++Runner)
	{
		float const Shown = FMath::Lerp(IsInEvenSet(Runner, Style.RunnerCount, Total),
										IsInEvenSet(Runner, Style.ActiveRunnerCount, Total), State.Activation);
		if (Shown > 0.f)
		{
			double const Distance = State.Travel + Length * Runner / Total;
			// The roll follows the unwrapped distance, so a runner never snaps back as it passes the start.
			float const Roll =
				static_cast<float>(FMath::Fmod(Style.RunnerRoll * 90.0 * Distance / Style.RunnerSize, 360.0));
			FVector2f const Position = GetPointAlongOutline(Outline, static_cast<float>(FMath::Fmod(Distance, Length)));
			TArray<FVector2f> const Square = MakePolygon(Position, Style.RunnerSize * UE_INV_SQRT_2, 4, 45.f + Roll);

			FLinearColor Color = Style.RunnerColor * LineTint;
			Color.A *= Shown * Opacity;
			if (Style.bAlternateHollowRunners && Runner % 2 == 1)
			{
				DrawOutline(OutDrawElements, LayerId + 2, Geometry, Square, Color, Style.HollowRunnerThickness);
			}
			else
			{
				DrawFilled(OutDrawElements, LayerId + 2, Geometry, Square, Color);
			}
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void FGeoShapePainter::AdvanceStyledState(UGeoFrameStyle const& Style, bool const bActive, float const DeltaTime,
										  FGeoOutlineState& InOutState)
{
	float const Target = bActive ? 1.f : 0.f;
	InOutState.Activation =
		Style.ActivationSeconds > 0.f
			? FMath::FInterpConstantTo(InOutState.Activation, Target, DeltaTime, 1.f / Style.ActivationSeconds)
			: Target;
	InOutState.Travel += DeltaTime * FMath::Lerp(Style.RunnerSpeed, Style.ActiveRunnerSpeed, InOutState.Activation);
}

// ---------------------------------------------------------------------------------------------------------------------
void FGeoShapePainter::DrawIcon(FSlateWindowElementList& OutDrawElements, int32 const LayerId,
								FGeometry const& Geometry, UGeoIcon const& Icon, FSlateRect const& Box,
								FLinearColor const& Tint)
{
	FVector2f const BoxSize(Box.GetSize());
	float const Scale = FMath::Min(BoxSize.X, BoxSize.Y) / Icon.ViewSize;
	FVector2f const Origin = FVector2f(Box.GetTopLeft()) + .5f * (BoxSize - FVector2f(Scale * Icon.ViewSize));
	TArray<FLinearColor> const Colors = GeoColor::GetMeaningColors(Icon.Color, Icon.SecondaryColors);

	for (FGeoIconStroke const& Stroke : Icon.Strokes)
	{
		if (!ensureMsgf(Colors.IsValidIndex(Stroke.ColorIndex), TEXT("%hs: %s has a stroke in colour %d of %d"),
						__FUNCTION__, *Icon.GetName(), Stroke.ColorIndex, Colors.Num()))
		{
			continue;
		}

		TArray<FVector2f> Points;
		Points.Reserve(Stroke.Points.Num());
		for (FVector2D const& Point : Stroke.Points)
		{
			Points.Add(Origin + Scale * FVector2f(Point));
		}

		FLinearColor Color = Colors[Stroke.ColorIndex] * Tint;
		Color.A *= Stroke.Opacity;
		if (Stroke.bFilled)
		{
			DrawFilled(OutDrawElements, LayerId, Geometry, Points, Color);
		}

		float const Thickness = Scale * Stroke.LineThickness;
		if (Stroke.DashLength <= 0.f)
		{
			DrawOutline(OutDrawElements, LayerId, Geometry, Points, Color, Thickness, Stroke.bClosed);
		}
		else
		{
			if (Stroke.bClosed && !Points.IsEmpty())
			{
				Points.Add(Points[0]);
			}
			for (TArray<FVector2f> const& Dash :
				 SplitIntoDashes(Points, Scale * Stroke.DashLength, Scale * Stroke.DashGap))
			{
				DrawOutline(OutDrawElements, LayerId, Geometry, Dash, Color, Thickness, false);
			}
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
TArray<TArray<FVector2f>> FGeoShapePainter::SplitIntoDashes(TArray<FVector2f> const& Points, float const DashLength,
															  float const GapLength)
{
	TArray<TArray<FVector2f>> Dashes;
	TArray<FVector2f> Dash;
	bool bInDash = true;
	float Left = DashLength;
	for (int32 Index = 0; Index + 1 < Points.Num(); ++Index)
	{
		FVector2f Start = Points[Index];
		FVector2f const End = Points[Index + 1];
		if (bInDash && Dash.IsEmpty())
		{
			Dash.Add(Start);
		}

		float SegmentLeft = FVector2f::Distance(Start, End);
		while (SegmentLeft > Left)
		{
			Start = FMath::Lerp(Start, End, Left / SegmentLeft);
			SegmentLeft -= Left;
			if (bInDash)
			{
				Dash.Add(Start);
				Dashes.Add(MoveTemp(Dash));
				Dash.Reset();
			}
			else
			{
				Dash.Add(Start);
			}
			bInDash = !bInDash;
			Left = bInDash ? DashLength : GapLength;
		}
		Left -= SegmentLeft;
		if (bInDash)
		{
			Dash.Add(End);
		}
	}

	if (Dash.Num() > 1)
	{
		Dashes.Add(MoveTemp(Dash));
	}
	return Dashes;
}
