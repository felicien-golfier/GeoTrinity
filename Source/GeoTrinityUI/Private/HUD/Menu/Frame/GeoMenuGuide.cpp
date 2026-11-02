// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/Frame/GeoMenuGuide.h"

namespace GeoMenuGuide
{
TArray<FSlateRect> MakeScrim(FSlateRect const& Canvas, FSlateRect const& Hole)
{
	FSlateRect const Inner = Hole.IntersectionWith(Canvas);
	TArray<FSlateRect> Scrim;
	for (FSlateRect const& Part : {FSlateRect(Canvas.Left, Canvas.Top, Canvas.Right, Inner.Top),
								   FSlateRect(Canvas.Left, Inner.Bottom, Canvas.Right, Canvas.Bottom),
								   FSlateRect(Canvas.Left, Inner.Top, Inner.Left, Inner.Bottom),
								   FSlateRect(Inner.Right, Inner.Top, Canvas.Right, Inner.Bottom)})
	{
		if (Part.Right > Part.Left && Part.Bottom > Part.Top)
		{
			Scrim.Add(Part);
		}
	}

	return Scrim;
}

FGeoMenuGuideHintPlace PlaceHint(FSlateRect const& Canvas, FSlateRect const& Hole, float const RoomNeeded,
								 float const Gap)
{
	bool const bAbove = Hole.Top - Canvas.Top >= RoomNeeded + Gap;
	bool const bRightHalf = (Hole.Left + Hole.Right) * .5f > (Canvas.Left + Canvas.Right) * .5f;
	FGeoMenuGuideHintPlace Place;
	Place.Position = FVector2f(bRightHalf ? Hole.Right : Hole.Left, bAbove ? Hole.Top - Gap : Hole.Bottom + Gap);
	Place.Alignment = FVector2f(bRightHalf ? 1.f : 0.f, bAbove ? 1.f : 0.f);
	return Place;
}
}  // namespace GeoMenuGuide
