// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
#include "EditorUtilityObject.h"

#include "GeoWidgetBuilderUtil.generated.h"

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

	/** Logs the full widget tree of a WidgetBlueprint — type, name, slot layout, and widget-specific properties. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void InspectWidgetBlueprint(UWidgetBlueprint* WidgetBlueprint);

private:
	static void LogWidget(UWidget* Widget, int32 Depth);
};

#endif // WITH_EDITOR
