// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FSlateResourceHandle;
class FSlateWindowElementList;
class UGeoFrameStyle;
class UGeoIcon;
struct FGeometry;

/** One moment of a styled outline: how far it has eased towards active and how far its runners have travelled. */
struct FGeoOutlineState
{
	/** 0 idle, 1 active. */
	float Activation = 0.f;
	/** Distance the runners have travelled along the outline, unwrapped. */
	double Travel = 0.0;
};

/** Flat geometric primitives every Geo style widget draws with, in the widget's local space. */
struct GEOTRINITYUI_API FGeoShapePainter
{
	/**
	 * Corners of a regular polygon around Center, the first one straight up turned by AngleDegrees clockwise. Fewer
	 * than 3 sides makes a circle.
	 */
	static TArray<FVector2f> MakePolygon(FVector2f Center, float Radius, int32 Sides, float AngleDegrees);

	/** Corners of the rectangle from Min to Max. */
	static TArray<FVector2f> MakeRectangle(FVector2f Min, FVector2f Max);

	/** Corners of a Size rectangle inset by Inset, each corner cut at 45 degrees by CornerCut. */
	static TArray<FVector2f> MakeCutRectangle(FVector2f Size, float Inset, float CornerCut);

	/** Point at Distance along the closed outline Points, wrapping past its end. */
	static FVector2f GetPointAlongOutline(TArray<FVector2f> const& Points, float Distance);

	/** The open line along the closed outline Points from its first corner, Length long. */
	static TArray<FVector2f> GetOutlinePart(TArray<FVector2f> const& Points, float Length);

	/** Length of the closed outline Points. */
	static float GetOutlineLength(TArray<FVector2f> const& Points);

	/** Fills the convex polygon Points, with an antialiased edge. */
	static void DrawFilled(FSlateWindowElementList& OutDrawElements, int32 LayerId, FGeometry const& Geometry,
						   TArray<FVector2f> const& Points, FLinearColor const& Color);

	/** Strokes the outline Points, joining its last corner back to the first when bClosed. */
	static void DrawOutline(FSlateWindowElementList& OutDrawElements, int32 LayerId, FGeometry const& Geometry,
							TArray<FVector2f> const& Points, FLinearColor const& Color, float Thickness,
							bool bClosed = true);

	/** Icon fitted into Box (centred, uniform scale), in the icon's colour multiplied by Tint. */
	static void DrawIcon(FSlateWindowElementList& OutDrawElements, int32 LayerId, FGeometry const& Geometry,
						 UGeoIcon const& Icon, FSlateRect const& Box, FLinearColor const& Tint);

	/** A soft stroke along the closed outline Points: nested strokes thinning towards the line, brightest on it. */
	static void DrawGlow(FSlateWindowElementList& OutDrawElements, int32 LayerId, FGeometry const& Geometry,
						 TArray<FVector2f> const& Points, FLinearColor const& Color, float Thickness);

	/** The fill of Style inside the convex Outline, on LayerId. Opacity multiplies every alpha. */
	static void PaintStyledFill(UGeoFrameStyle const& Style, FGeoOutlineState const& State, float Opacity,
								TArray<FVector2f> const& Outline, FGeometry const& Geometry,
								FSlateWindowElementList& OutDrawElements, int32 LayerId);

	/** The glow, line and runners of Style along Outline, on LayerId to LayerId + 2. Opacity multiplies every alpha,
	 * GlowTint the glow's colour. */
	static void PaintStyledLine(UGeoFrameStyle const& Style, FGeoOutlineState const& State, float Opacity,
								TArray<FVector2f> const& Outline, FGeometry const& Geometry,
								FSlateWindowElementList& OutDrawElements, int32 LayerId,
								FLinearColor const& GlowTint = FLinearColor::White);

	/** State eased one step of DeltaTime towards bActive, its runners moved on at the speed Style gives that state. */
	static void AdvanceStyledState(UGeoFrameStyle const& Style, bool bActive, float DeltaTime,
								   FGeoOutlineState& InOutState);

private:
	/** Points cut into DashLength pieces with GapLength between them, each piece an open polyline. */
	static TArray<TArray<FVector2f>> SplitIntoDashes(TArray<FVector2f> const& Points, float DashLength,
													 float GapLength);

	/** The plain white texture custom-vertex shapes are drawn with, tinted by their vertex colours. */
	static FSlateResourceHandle GetWhiteTexture();
};
