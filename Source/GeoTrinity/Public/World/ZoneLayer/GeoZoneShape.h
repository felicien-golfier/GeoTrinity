// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Containers/StaticArray.h"
#include "CoreMinimal.h"

#include "GeoZoneShape.generated.h"

struct FGeoColorParam;
class UGeoZoneVisualComponent;

/** What a zone shape is; its ordinal is the shape the zone layer's HLSL branches on (zone_layer_hlsl.py). */
UENUM()
enum class EGeoZoneShape : uint8
{
	/** Radius round the component. */
	Circle,
	/** Length along the component's +X, HalfWidth each side. */
	Beam
};

/** Which plane of the zone layer draws a shape: the ground's meaning pattern, or the telegraph stripes above it. */
UENUM()
enum class EGeoZoneLayer : uint8
{
	Ground,
	Telegraph
};

/** One shape's size, in the local space of the component drawing it, world cm. Each shape reads its own fields. */
USTRUCT()
struct FGeoZoneShape
{
	GENERATED_BODY()

	UPROPERTY()
	EGeoZoneShape Shape = EGeoZoneShape::Circle;

	/** Circle. */
	UPROPERTY()
	float Radius = 0.f;

	/** Beam. */
	UPROPERTY()
	float Length = 0.f;

	/** Beam. */
	UPROPERTY()
	float HalfWidth = 0.f;
};

/** The zone layer's slot format, shared by C++ (GeoZoneLayer::PackSlot) and the materials (M_ZoneLayer). */
namespace GeoZoneLayer
{
	/** Most shapes drawn at once; a texture this many texels wide. */
	constexpr int32 MaxZoneSlots = 128;
	/** Texel rows per slot: the head (shape, colours, flags, fill), then the shape's 8 world-space floats. */
	constexpr int32 SlotRows = 3;
	/** World Z of both layer planes, just over the floor. */
	constexpr float ZoneLayerZ = 1.f;
	/** Translucency sort priority of each plane: the telegraphs draw over the ground. */
	constexpr int32 GroundSortPriority = 0;
	constexpr int32 TelegraphSortPriority = 1;
	/** Bits of a slot's Flags texel. */
	constexpr uint32 TelegraphFlag = 1;
	constexpr uint32 SafeSpotCutFlag = 2;

	/** One slot's texels, one per row. */
	using FSlotTexels = TStaticArray<FLinearColor, SlotRows>;

	/**
	 * Component's slot as drawn now: row 0 = (Shape ordinal, ColorMask, Flags, GetFill()), rows 1–2 = the shape at
	 * Component's world transform (circle: centre XY, radius; beam: start XY, end XY, half-width).
	 */
	GEOTRINITY_API FSlotTexels PackSlot(UGeoZoneVisualComponent const& Component);

	/** One bit per EGeoColor ordinal, Color first then SecondaryColors, duplicates merged. Override carries no palette
	 * colour: ensures and drops its bit. */
	GEOTRINITY_API uint32 MakeColorMask(FGeoColorParam const& Color, TArray<FGeoColorParam> const& SecondaryColors);
} // namespace GeoZoneLayer
