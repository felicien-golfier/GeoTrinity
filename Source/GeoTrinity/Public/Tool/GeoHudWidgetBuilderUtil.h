// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
#include "EditorUtilityObject.h"

#include "GeoHudWidgetBuilderUtil.generated.h"

class UUserWidget;
class UWidgetBlueprint;

/**
 * Content-specific HUD widget-tree builders. Each function builds one named widget tree by composing the generic
 * primitives in UGeoWidgetBuilderUtil. New per-widget builders go here, keeping the generic util uncluttered.
 */
UCLASS()
class GEOTRINITY_API UGeoHudWidgetBuilderUtil : public UEditorUtilityObject
{
	GENERATED_BODY()

public:
	/**
	 * Builds the WBP_AbilitySlot tree: an Overlay root holding a SquareSize x SquareSize SizeBox ("Square") that contains
	 *   Icon (Image) ← CooldownSweep (Image) ← CountdownText (centered) ← CountText (bottom-right corner).
	 * The fixed square keeps the slot a stable size regardless of icon/cooldown content (resolution scaling is handled by
	 * the bar's fractional anchors + UMG DPI). Names match the BindWidget members on UGeoAbilitySlotWidget. Saves the asset.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void BuildAbilitySlotWidget(UWidgetBlueprint* WidgetBlueprint, float SquareSize = 64.f);

	/**
	 * Builds the WBP_AbilityBar tree: an Overlay root holding the SlotBox HorizontalBox horizontally centered. Centering
	 * the box (rather than stretching it) keeps the run of slots packed and centered in the bar region. SlotBox matches
	 * the BindWidget on UGeoAbilityBarWidget, which fills it with slots at runtime. Compiles and saves the asset.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void BuildAbilityBarWidget(UWidgetBlueprint* WidgetBlueprint);

	/**
	 * Builds the WBP_ChargeBeamGauge widget tree:
	 *   Root: CanvasPanel — ChargeBar (full, dark-blue) + SweetSpotBar (golden, at SweetSpotMin..Max).
	 * Compiles and saves the asset.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void BuildChargeBeamGaugeWidget(UWidgetBlueprint* WidgetBlueprint, float SweetSpotMinRatio = 0.6f,
										   float SweetSpotMaxRatio = 0.7f);

	/**
	 * Adds AbilityBarClass into the overlay's CanvasPanel ParentPanelName as a child named "AbilityBar" (matching the
	 * BindWidget on UGeoOverlayWidget), anchored bottom-center. Sizing is expressed as fractions of the canvas so the bar
	 * occupies the same screen proportion at any resolution: WidthFraction/HeightFraction of the canvas, lifted
	 * BottomFraction of the canvas height off the bottom edge. Composes UGeoWidgetBuilderUtil::AddChildToCanvasPanel;
	 * compiles and saves the asset.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void AddAbilityBarToOverlay(UWidgetBlueprint* WidgetBlueprint, FName ParentPanelName,
									   TSubclassOf<UUserWidget> AbilityBarClass, float WidthFraction = 0.3f,
									   float HeightFraction = 0.08f, float BottomFraction = 0.02f);

	/**
	 * Builds the WBP_MainMenu connect-screen tree: a centered VerticalBox holding
	 *   TitleText ("GeoTrinity") ← HostButton (label "Host") ← IPInput (EditableTextBox, hint "Host IP")
	 *   ← JoinButton (label "Join") ← LocalIPText (host reads this out; bound at runtime to GetLocalIP).
	 * The child names match the BindWidget members the BP exposes so UGeoMenuWidget's graph can wire them. Saves the asset.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void BuildMainMenuWidget(UWidgetBlueprint* WidgetBlueprint);
};

#endif // WITH_EDITOR
