// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Tutorial/SGeoCatFace.h"

#include "HUD/Style/Glyph/GeoShapePainter.h"
#include "HUD/Tutorial/GeoCatFaceMotion.h"

constexpr float DesignSize = 200.f;
constexpr int32 CurveSegments = 8;
constexpr float InnerEarOpacity = .6f;
constexpr float NoseOpacity = .9f;
constexpr float EyeHaloOpacity = .5f;
constexpr float EyeHaloThickness = 6.f;
constexpr float MouthThickness = 3.f;
constexpr float EyeTiltDegrees = 9.f;
constexpr float EyeCenterY = 106.f;
constexpr float EyeLeftX = 70.f;
constexpr float EyeRightX = 130.f;
constexpr float EarBaseY = 56.f;
constexpr float MouthTopY = 138.f;
constexpr float CornerRadius = 44.f;

// ---------------------------------------------------------------------------------------------------------------------
/** Adds the points of the quadratic curve From, Control, To after From, up to and including To. */
static void AppendCurve(TArray<FVector2f>& InOutPoints, FVector2f const& From, FVector2f const& Control,
						FVector2f const& To)
{
	for (int32 Segment = 1; Segment <= CurveSegments; ++Segment)
	{
		float const T = static_cast<float>(Segment) / CurveSegments;
		InOutPoints.Add(FMath::Square(1.f - T) * From + 2.f * (1.f - T) * T * Control + FMath::Square(T) * To);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void SGeoCatFace::Construct(FArguments const& /*InArgs*/)
{
	RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SGeoCatFace::Animate));
}

// ---------------------------------------------------------------------------------------------------------------------
void SGeoCatFace::SetBase(FLinearColor const& InBase)
{
	Base = InBase;
	Invalidate(EInvalidateWidgetReason::Paint);
}

// ---------------------------------------------------------------------------------------------------------------------
void SGeoCatFace::SetShade(FLinearColor const& InShade)
{
	Shade = InShade;
	Invalidate(EInvalidateWidgetReason::Paint);
}

// ---------------------------------------------------------------------------------------------------------------------
void SGeoCatFace::SetInk(FLinearColor const& InInk)
{
	Ink = InInk;
	Invalidate(EInvalidateWidgetReason::Paint);
}

// ---------------------------------------------------------------------------------------------------------------------
void SGeoCatFace::SetLight(FLinearColor const& InLight)
{
	Light = InLight;
	Invalidate(EInvalidateWidgetReason::Paint);
}

// ---------------------------------------------------------------------------------------------------------------------
void SGeoCatFace::SetSize(float const InSize)
{
	Size = InSize;
	Invalidate(EInvalidateWidgetReason::Layout);
}

// ---------------------------------------------------------------------------------------------------------------------
void SGeoCatFace::SetTalking(bool const bInTalking)
{
	bTalking = bInTalking;
	Invalidate(EInvalidateWidgetReason::Paint);
}

// ---------------------------------------------------------------------------------------------------------------------
FVector2D SGeoCatFace::ComputeDesiredSize(float /*LayoutScaleMultiplier*/) const
{
	return FVector2D(Size, Size);
}

// ---------------------------------------------------------------------------------------------------------------------
EActiveTimerReturnType SGeoCatFace::Animate(double const CurrentTime, float /*DeltaTime*/)
{
	Seconds = CurrentTime;
	Invalidate(EInvalidateWidgetReason::Paint);
	return EActiveTimerReturnType::Continue;
}

// ---------------------------------------------------------------------------------------------------------------------
TArray<FVector2f> SGeoCatFace::Place(TArray<FVector2f> const& DesignPoints, FGeometry const& Geometry,
									 FVector2f const Pivot, float const AngleDegrees, float const ScaleY) const
{
	FVector2f const LocalSize = Geometry.GetLocalSize();
	float const FitScale = FMath::Min(LocalSize.X, LocalSize.Y) / DesignSize;
	FVector2f const Origin = (LocalSize - FVector2f(DesignSize, DesignSize) * FitScale) * .5f;
	float const Cosine = FMath::Cos(FMath::DegreesToRadians(AngleDegrees));
	float const Sine = FMath::Sin(FMath::DegreesToRadians(AngleDegrees));

	TArray<FVector2f> Placed;
	Placed.Reserve(DesignPoints.Num());
	for (FVector2f const& Point : DesignPoints)
	{
		FVector2f const Offset(Point.X - Pivot.X, (Point.Y - Pivot.Y) * ScaleY);
		FVector2f const Turned(Offset.X * Cosine - Offset.Y * Sine, Offset.X * Sine + Offset.Y * Cosine);
		Placed.Add(Origin + (Pivot + Turned) * FitScale);
	}

	return Placed;
}

// ---------------------------------------------------------------------------------------------------------------------
void SGeoCatFace::DrawShape(FSlateWindowElementList& OutDrawElements, int32 const LayerId, FGeometry const& Geometry,
							TArray<FVector2f> const& Rim, FLinearColor const& Color) const
{
	FVector2f Center = FVector2f::ZeroVector;
	for (FVector2f const& Corner : Rim)
	{
		Center += Corner / Rim.Num();
	}

	TArray<FVector2f> Fan;
	Fan.Reserve(Rim.Num() + 2);
	Fan.Add(Center);
	Fan.Append(Rim);
	Fan.Add(Rim[0]);
	FGeoShapePainter::DrawFilled(OutDrawElements, LayerId, Geometry, Fan, Color);
}

// ---------------------------------------------------------------------------------------------------------------------
int32 SGeoCatFace::OnPaint(FPaintArgs const& /*Args*/, FGeometry const& AllottedGeometry,
						   FSlateRect const& /*MyCullingRect*/, FSlateWindowElementList& OutDrawElements,
						   int32 const LayerId, FWidgetStyle const& InWidgetStyle, bool /*bParentEnabled*/) const
{
	FLinearColor const Tint = InWidgetStyle.GetColorAndOpacityTint();
	FLinearColor const BaseColor = Base * Tint;
	FLinearColor const InkColor = Ink * Tint;
	FLinearColor const LightColor = Light * Tint;
	FLinearColor InnerEarColor = Shade * Tint;
	InnerEarColor.A *= InnerEarOpacity;
	FLinearColor NoseColor = LightColor;
	NoseColor.A *= NoseOpacity;
	FLinearColor EyeHaloColor = LightColor;
	EyeHaloColor.A *= EyeHaloOpacity;
	float const LayoutScale =
		FMath::Min(AllottedGeometry.GetLocalSize().X, AllottedGeometry.GetLocalSize().Y) / DesignSize;

	TArray<FVector2f> const OuterEars[] = {{{44.f, 60.f}, {56.f, 16.f}, {96.f, 52.f}},
										   {{156.f, 60.f}, {144.f, 16.f}, {104.f, 52.f}}};
	TArray<FVector2f> const InnerEars[] = {{{58.f, 26.f}, {70.f, 56.f}, {50.f, 54.f}},
										   {{142.f, 26.f}, {130.f, 56.f}, {150.f, 54.f}}};
	FVector2f const EarPivots[] = {{EyeLeftX, EarBaseY}, {EyeRightX, EarBaseY}};
	for (int32 EarIndex = 0; EarIndex < UE_ARRAY_COUNT(EarPivots); ++EarIndex)
	{
		float const Angle = GeoCatFaceMotion::GetEarAngle(Seconds, EarIndex);
		DrawShape(OutDrawElements, LayerId, AllottedGeometry,
				  Place(OuterEars[EarIndex], AllottedGeometry, EarPivots[EarIndex], Angle), BaseColor);
		DrawShape(OutDrawElements, LayerId + 1, AllottedGeometry,
				  Place(InnerEars[EarIndex], AllottedGeometry, EarPivots[EarIndex], Angle), InnerEarColor);
	}

	TArray<FVector2f> const HeadCorners = {
		{38.f, 64.f},	{162.f, 64.f}, {178.f, 104.f}, {168.f, 112.f}, {180.f, 128.f}, {166.f, 138.f}, {170.f, 152.f},
		{100.f, 176.f}, {30.f, 152.f}, {34.f, 138.f},  {20.f, 128.f},  {32.f, 112.f},  {22.f, 104.f}};
	DrawShape(OutDrawElements, LayerId + 2, AllottedGeometry, Place(HeadCorners, AllottedGeometry), BaseColor);

	// The rectangle x22 y60 w156 h96 r44 as four quarter circles, bottom right first.
	FVector2f const CornerCenters[] = {{134.f, 112.f}, {66.f, 112.f}, {66.f, 104.f}, {134.f, 104.f}};
	TArray<FVector2f> Rounded;
	for (int32 Corner = 0; Corner < UE_ARRAY_COUNT(CornerCenters); ++Corner)
	{
		for (int32 Step = 0; Step <= CurveSegments; ++Step)
		{
			float const Angle = FMath::DegreesToRadians(90.f * (Corner + static_cast<float>(Step) / CurveSegments));
			Rounded.Add(CornerCenters[Corner] + CornerRadius * FVector2f(FMath::Cos(Angle), FMath::Sin(Angle)));
		}
	}
	DrawShape(OutDrawElements, LayerId + 2, AllottedGeometry, Place(Rounded, AllottedGeometry), BaseColor);

	float const EyeScale = GeoCatFaceMotion::GetEyeScale(Seconds);
	for (int32 EyeIndex = 0; EyeIndex < 2; ++EyeIndex)
	{
		float const X = EyeIndex == 0 ? EyeLeftX : EyeRightX;
		FVector2f const BottomLeft(X - 9.f, 120.f);
		FVector2f const BottomRight(X + 9.f, 120.f);
		TArray<FVector2f> EyeShape = {BottomLeft, {X - 9.f, 101.f}};
		AppendCurve(EyeShape, {X - 9.f, 101.f}, {X - 9.f, 92.f}, {X, 92.f});
		AppendCurve(EyeShape, {X, 92.f}, {X + 9.f, 92.f}, {X + 9.f, 101.f});
		EyeShape.Add(BottomRight);
		AppendCurve(EyeShape, BottomRight, {X, 102.6f}, BottomLeft);

		TArray<FVector2f> const Eye = Place(EyeShape, AllottedGeometry, {X, EyeCenterY},
											EyeIndex == 0 ? EyeTiltDegrees : -EyeTiltDegrees, EyeScale);
		FGeoShapePainter::DrawGlow(OutDrawElements, LayerId + 3, AllottedGeometry, Eye, EyeHaloColor,
								   EyeHaloThickness * LayoutScale);
		DrawShape(OutDrawElements, LayerId + 4, AllottedGeometry, Eye, LightColor);
	}

	DrawShape(OutDrawElements, LayerId + 4, AllottedGeometry,
			  Place({{95.f, 128.f}, {105.f, 128.f}, {100.f, 134.f}}, AllottedGeometry), NoseColor);

	if (bTalking)
	{
		TArray<FVector2f> OpenMouth = {{87.f, MouthTopY}};
		AppendCurve(OpenMouth, {87.f, MouthTopY}, {100.f, 166.f}, {113.f, MouthTopY});
		TArray<FVector2f> const Mouth =
			Place(OpenMouth, AllottedGeometry, {100.f, MouthTopY}, 0.f, GeoCatFaceMotion::GetMouthScale(Seconds));
		DrawShape(OutDrawElements, LayerId + 4, AllottedGeometry, Mouth, InkColor);
		FGeoShapePainter::DrawOutline(OutDrawElements, LayerId + 4, AllottedGeometry, Mouth, InkColor,
									  MouthThickness * LayoutScale);
	}
	else
	{
		TArray<FVector2f> ClosedMouth = {{86.f, 139.f}};
		AppendCurve(ClosedMouth, {86.f, 139.f}, {93.f, 149.f}, {100.f, 139.f});
		AppendCurve(ClosedMouth, {100.f, 139.f}, {107.f, 149.f}, {114.f, 139.f});
		FGeoShapePainter::DrawOutline(OutDrawElements, LayerId + 4, AllottedGeometry,
									  Place(ClosedMouth, AllottedGeometry), InkColor, MouthThickness * LayoutScale,
									  false);
	}

	return LayerId + 5;
}
