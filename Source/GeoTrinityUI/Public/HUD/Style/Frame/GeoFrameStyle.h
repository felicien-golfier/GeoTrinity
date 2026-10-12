// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "GeoFrameStyle.generated.h"

/**
 * The look of a UGeoFrame: an outline with cut corners, an optional glow under it, a fill, an optional grid, and small
 * squares ("runners") travelling around the outline. A frame is idle or active — hovered, focused or selected — and
 * eases between the two looks, so every value with an Active twin is the active end of that blend.
 * One asset per kind of frame (button, panel, row, field, screen); every frame wearing it changes with it.
 */
UCLASS(BlueprintType)
class GEOTRINITYUI_API UGeoFrameStyle : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoFrame|Line")
	FLinearColor LineColor = FLinearColor(1.f, 1.f, 1.f, .35f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoFrame|Line")
	FLinearColor ActiveLineColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoFrame|Line", meta = (ClampMin = "0", ClampMax = "16"))
	float LineThickness = 1.5f;

	/** Length cut off each corner at 45 degrees; 0 keeps square corners. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoFrame|Line", meta = (ClampMin = "0", ClampMax = "64"))
	float CornerCut = 0.f;

	/** Wide soft line drawn under the outline. Transparent or zero thickness draws none. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoFrame|Glow")
	FLinearColor GlowColor = FLinearColor(.45f, .2f, 1.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoFrame|Glow")
	FLinearColor ActiveGlowColor = FLinearColor(.45f, .2f, 1.f, .25f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoFrame|Glow", meta = (ClampMin = "0", ClampMax = "32"))
	float GlowThickness = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoFrame|Fill")
	FLinearColor FillColor = FLinearColor::Transparent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoFrame|Fill")
	FLinearColor ActiveFillColor = FLinearColor(1.f, 1.f, 1.f, .05f);

	/** Spacing of a square grid drawn inside the fill; 0 draws none. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoFrame|Fill", meta = (ClampMin = "0", ClampMax = "512"))
	float GridSpacing = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoFrame|Fill")
	FLinearColor GridColor = FLinearColor(1.f, 1.f, 1.f, .03f);

	/** Squares travelling the outline while idle, evenly spaced. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoFrame|Runners", meta = (ClampMin = "0", ClampMax = "32"))
	int32 RunnerCount = 1;

	/** Squares travelling the outline while active. A multiple of RunnerCount keeps both sets evenly spaced. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoFrame|Runners", meta = (ClampMin = "0", ClampMax = "32"))
	int32 ActiveRunnerCount = 2;

	/** Side of a runner square. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoFrame|Runners", meta = (ClampMin = "1", ClampMax = "64"))
	float RunnerSize = 6.f;

	/** Travel speed along the outline while idle, in pixels per second, so a big frame turns as fast as a small one. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoFrame|Runners", meta = (ClampMin = "0", ClampMax = "4000"))
	float RunnerSpeed = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoFrame|Runners", meta = (ClampMin = "0", ClampMax = "4000"))
	float ActiveRunnerSpeed = 160.f;

	/** How much a runner turns as it travels: 1 rolls it like a square on the line, 0 keeps it upright. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoFrame|Runners", meta = (ClampMin = "0", ClampMax = "4"))
	float RunnerRoll = 1.f;

	/** Every other runner is drawn hollow, the rest solid. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoFrame|Runners")
	bool bAlternateHollowRunners = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoFrame|Runners")
	FLinearColor RunnerColor = FLinearColor::White;

	/** Outline thickness of a hollow runner. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoFrame|Runners", meta = (ClampMin = "0.5", ClampMax = "8"))
	float HollowRunnerThickness = 1.5f;

	/** Time to ease from idle to active and back. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoFrame|Motion", meta = (ClampMin = "0", ClampMax = "2"))
	float ActivationSeconds = .15f;

	/** Opacity of the whole frame while its widget is disabled. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoFrame|Motion", meta = (ClampMin = "0", ClampMax = "1"))
	float DisabledOpacity = .35f;
};
