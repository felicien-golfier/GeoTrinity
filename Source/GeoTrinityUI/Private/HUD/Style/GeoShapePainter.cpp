// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Style/GeoShapePainter.h"

#include "Framework/Application/SlateApplication.h"
#include "HUD/Style/GeoFrameStyle.h"
#include "Layout/Geometry.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"

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
void FGeoShapePainter::DrawFilled(FSlateWindowElementList& OutDrawElements, int32 const LayerId,
								  FGeometry const& Geometry, TArray<FVector2f> const& Points, FLinearColor const& Color)
{
	if (Points.Num() < 3 || Color.A <= 0.f)
	{
		return;
	}

	FSlateResourceHandle const WhiteTexture =
		FSlateApplication::Get().GetRenderer()->GetResourceHandle(*FCoreStyle::Get().GetBrush("WhiteBrush"));
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
	FSlateDrawElement::MakeCustomVerts(OutDrawElements, LayerId, WhiteTexture, Vertices, Indices, nullptr, 0, 0);

	// The fan's edge is aliased; a hairline of the same colour over it smooths it.
	DrawOutline(OutDrawElements, LayerId, Geometry, Points, Color, 1.f);
}

// ---------------------------------------------------------------------------------------------------------------------
void FGeoShapePainter::DrawOutline(FSlateWindowElementList& OutDrawElements, int32 const LayerId,
								   FGeometry const& Geometry, TArray<FVector2f> const& Points,
								   FLinearColor const& Color, float const Thickness)
{
	if (Points.Num() < 2 || Color.A <= 0.f || Thickness <= 0.f)
	{
		return;
	}

	TArray<FVector2f> Closed = Points;
	Closed.Add(Points[0]);
	FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(), MoveTemp(Closed),
								 ESlateDrawEffect::None, Color, true, Thickness);
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
									   FSlateWindowElementList& OutDrawElements, int32 const LayerId)
{
	auto const Blend = [&State, Opacity](FLinearColor const& Idle, FLinearColor const& Active)
	{
		FLinearColor Color = FMath::Lerp(Idle, Active, State.Activation);
		Color.A *= Opacity;
		return Color;
	};
	DrawGlow(OutDrawElements, LayerId, Geometry, Outline, Blend(Style.GlowColor, Style.ActiveGlowColor),
			 Style.GlowThickness);
	DrawOutline(OutDrawElements, LayerId + 1, Geometry, Outline, Blend(Style.LineColor, Style.ActiveLineColor),
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

			FLinearColor Color = Style.RunnerColor;
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
