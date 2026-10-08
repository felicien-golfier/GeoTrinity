// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Tool/GeoColor.h"

#include "GeoIcon.generated.h"

/** One line of a UGeoIcon: a polyline, open or closed, stroked, filled, or both. */
USTRUCT(BlueprintType)
struct FGeoIconStroke
{
	GENERATED_BODY()

	/** Corners, in the icon's ViewSize box (0,0 top-left). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoIcon")
	TArray<FVector2D> Points;

	/** Joins the last corner back to the first. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoIcon")
	bool bClosed = false;

	/** Fills the shape solid. Only convex shapes fill correctly. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoIcon")
	bool bFilled = false;

	/** Line width in ViewSize units; 0 draws no line. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoIcon", meta = (ClampMin = "0", ClampMax = "16"))
	float LineThickness = 2.f;

	/** Faint strokes are the secondary part of the icon: the trail, the motion, the echo. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoIcon", meta = (ClampMin = "0", ClampMax = "1"))
	float Opacity = 1.f;

	/** Dash and gap lengths in ViewSize units; a zero dash draws the line whole. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoIcon", meta = (ClampMin = "0", ClampMax = "44"))
	float DashLength = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoIcon", meta = (ClampMin = "0", ClampMax = "44"))
	float DashGap = 0.f;

	/** The meaning it is drawn in: 0 is the icon's Color, 1 and on its SecondaryColors in order. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoIcon", meta = (ClampMin = "0", ClampMax = "3"))
	int32 ColorIndex = 0;
};

/**
 * A flat vector icon — an ability, a status — drawn by the UI with Slate lines and fills, crisp at any size. Each
 * stroke is drawn in one game colour, a meaning: the icon's Color, or one of its SecondaryColors for an ability doing
 * several things (a beam that damages enemies and heals allies). Faint strokes only vary in opacity. Data only, so gameplay assets can point at it on every
 * target, the dedicated server included. Built from the SVGs in SourceArt/Icons by AI/Python/UI/import_icons.py.
 */
UCLASS(BlueprintType)
class GEOTRINITY_API UGeoIcon : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoIcon")
	FGeoColorParam Color;

	/** The icon's other meanings, picked by a stroke's ColorIndex from 1. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoIcon")
	TArray<FGeoColorParam> SecondaryColors;

	/** Side of the square box the stroke corners are given in. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoIcon", meta = (ClampMin = "1"))
	float ViewSize = 44.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoIcon")
	TArray<FGeoIconStroke> Strokes;
};
