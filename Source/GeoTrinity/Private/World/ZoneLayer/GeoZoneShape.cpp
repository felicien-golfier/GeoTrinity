// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "World/ZoneLayer/GeoZoneShape.h"

#include "Tool/Visual/GeoColor.h"
#include "World/ZoneLayer/GeoZoneVisualComponent.h"

static_assert(GeoColor::SlotCount <= 16,
			  "The zone layer's HLSL reads 16 colour bits: COLOR_BITS in zone_layer_hlsl.py");

GeoZoneLayer::FSlotTexels GeoZoneLayer::PackSlot(UGeoZoneVisualComponent const& Component)
{
	uint32 Flags = 0;
	if (Component.Layer == EGeoZoneLayer::Telegraph)
	{
		Flags |= TelegraphFlag;
	}

	if (Component.bSafeSpotCut)
	{
		Flags |= SafeSpotCutFlag;
	}

	FTransform const& Transform = Component.GetComponentTransform();
	FVector2D const Origin(Transform.GetLocation());
	FSlotTexels Texels(InPlace, FLinearColor::Transparent);
	Texels[0] = FLinearColor(static_cast<float>(Component.Shape.Shape), static_cast<float>(Component.ColorMask),
							 static_cast<float>(Flags), Component.GetFill());
	switch (Component.Shape.Shape)
	{
	case EGeoZoneShape::Circle:
		Texels[1] = FLinearColor(Origin.X, Origin.Y, Component.Shape.Radius, 0.f);
		break;
	case EGeoZoneShape::Beam:
		{
			FVector2D const Forward = FVector2D(Transform.GetUnitAxis(EAxis::X)).GetSafeNormal();
			FVector2D const End = Origin + Forward * Component.Shape.Length;
			Texels[1] = FLinearColor(Origin.X, Origin.Y, End.X, End.Y);
			Texels[2] = FLinearColor(Component.Shape.HalfWidth, 0.f, 0.f, 0.f);
			break;
		}
	}

	return Texels;
}

uint32 GeoZoneLayer::MakeColorMask(FGeoColorParam const& Color, TArray<FGeoColorParam> const& SecondaryColors)
{
	uint32 Mask = 0;
	auto AddColor = [&Mask](FGeoColorParam const& Param)
	{
		if (ensureMsgf(Param.Color != EGeoColor::Override,
					   TEXT("GeoZoneLayer: a zone shape names EGeoColor::Override, which has no palette slot to draw")))
		{
			Mask |= 1u << static_cast<uint32>(Param.Color);
		}
	};

	AddColor(Color);
	for (FGeoColorParam const& SecondaryColor : SecondaryColors)
	{
		AddColor(SecondaryColor);
	}

	return Mask;
}
