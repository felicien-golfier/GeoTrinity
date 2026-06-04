// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
#include "EditorUtilityObject.h"

#include "GeoWidgetBuilderUtil.generated.h"

class UMaterialInterface;
class UWidgetBlueprint;
class UWidget;

UCLASS()
class GEOTRINITY_API UGeoWidgetBuilderUtil : public UEditorUtilityObject
{
	GENERATED_BODY()

public:
	/**
	 * Builds the WBP_ChargeBeamGauge widget tree:
	 *   Root: CanvasPanel (300×30, fills designer canvas)
	 *     - ChargeBar     : ProgressBar, full width, dark-blue fill
	 *     - SweetSpotBar  : ProgressBar, overlaid at SweetSpotMinRatio..SweetSpotMaxRatio, golden fill
	 * Saves the asset after building.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void BuildChargeBeamGaugeWidget(UWidgetBlueprint* WidgetBlueprint, float SweetSpotMinRatio = 0.6f,
	                                       float SweetSpotMaxRatio = 0.7f);

	/**
	 * Generic: replaces the widget's root with a single Image showing Texture at DesiredSize.
	 * Any single-image widget (software cursor, icon, …) is composed by the caller from this primitive.
	 * Compiles and saves the asset.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void SetImageRoot(UWidgetBlueprint* WidgetBlueprint, UTexture2D* Texture, FVector2D DesiredSize);

	/**
	 * Generic: replaces the widget's root with a single Image drawing Material at DesiredSize.
	 * Use when the brush needs shader logic (e.g. luminance-to-alpha masking) rather than a raw texture.
	 * Compiles and saves the asset.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void SetImageRootFromMaterial(UWidgetBlueprint* WidgetBlueprint, UMaterialInterface* Material,
										 FVector2D DesiredSize);

	/** Logs the full widget tree of a WidgetBlueprint — type, name, slot layout, and widget-specific properties. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void InspectWidgetBlueprint(UWidgetBlueprint* WidgetBlueprint);

private:
	/** Validates the blueprint and its tree, marks both for modification, and clears the root. Returns null on failure. */
	static class UWidgetTree* BeginBuild(UWidgetBlueprint* WidgetBlueprint, TCHAR const* FunctionName);

	/** Compiles and saves the widget blueprint after its tree has been built. */
	static void FinishBuild(UWidgetBlueprint* WidgetBlueprint);

	static void LogWidget(UWidget* Widget, int32 Depth);
};

#endif // WITH_EDITOR
