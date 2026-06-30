// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityObject.h"

#include "GeoHudWidgetBuilderUtil.generated.h"

class UUserWidget;
class UWidgetBlueprint;

/** Where the ability slot's live key-binding label (KeyText) is placed when building WBP_AbilitySlot. */
UENUM()
enum class EAbilitySlotKeyLabelPlacement : uint8
{
	/** KeyText sits in a VerticalBox just under the icon square. */
	Below,
	/** KeyText overlays the icon, bottom-center, inside the square. */
	OverlayBottom,
	/** No KeyText is built. */
	None
};

/**
 * Content-specific HUD widget-tree builders. Each function builds one named widget tree by composing the generic
 * primitives in UGeoWidgetBuilderUtil. New per-widget builders go here, keeping the generic util uncluttered.
 */
UCLASS()
class GEOTRINITYEDITOR_API UGeoHudWidgetBuilderUtil : public UEditorUtilityObject
{
	GENERATED_BODY()

public:
	/**
	 * Builds the WBP_AbilitySlot tree. A SquareSize x SquareSize SizeBox ("Square") holds
	 *   Icon (Image) ← CooldownSweep (Image) ← CountdownText (centered) ← CountText (bottom-right corner).
	 * KeyLabelPlacement decides where the live key-binding label (KeyText, e.g. "LMB", refreshed from C++ each tick) goes:
	 * Below puts it under the square in a VerticalBox root; OverlayBottom stacks it bottom-center over the icon (Overlay
	 * root); None omits it (Overlay root).
	 * The fixed square keeps the slot a stable size regardless of icon/cooldown content (resolution scaling is handled by
	 * the bar's fractional anchors + UMG DPI). Names match the BindWidget members on UGeoAbilitySlotWidget. Saves the asset.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void BuildAbilitySlotWidget(UWidgetBlueprint* WidgetBlueprint, float SquareSize = 64.f,
									   EAbilitySlotKeyLabelPlacement KeyLabelPlacement = EAbilitySlotKeyLabelPlacement::Below);

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
	 * Builds the WBP_CombattantLifeBar tree: a SizeBox (BarWidth x BarHeight) → Overlay → HealthBar (fill) under
	 * ShieldBar (fill, semi-transparent cyan). Both progress bars fill the same rect so the shield overlays the health,
	 * mirroring WBP_MainOverlay. Names match the BindWidgetOptional members on UGenericCombattantWidget; the health color
	 * is driven at runtime by UpdateHealthRatio, and the shield percent by UpdateShieldRatio (Shield / MaxHealth).
	 * Compiles and saves the asset.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void BuildCombattantLifeBarWidget(UWidgetBlueprint* WidgetBlueprint, float BarWidth = 100.f,
											 float BarHeight = 12.f);

	/**
	 * Builds the WBP_LocalConnect tree (the "Play Local" direct-IP panel): an Overlay root with a centered VerticalBox
	 *   HostButton ← IPInput (EditableTextBox, hint "Host IP") ← JoinButton ← LocalIPText ← BackButton.
	 * Buttons are MenuButtonClass instances (a UGeoMenuButton WBP, e.g. WBP_GeoButton) with per-instance labels, so they
	 * satisfy the UGeoMenuButton BindWidgets on UGeoLocalConnectWidget. Compiles and saves the asset.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void BuildLocalConnectWidget(UWidgetBlueprint* WidgetBlueprint, TSubclassOf<UUserWidget> MenuButtonClass);

	/**
	 * Appends the "Play Local" entry to the existing main-menu tree WITHOUT rebuilding it: inserts PlayLocalButton
	 * (a MenuButtonClass instance) plus a spacer just above QuitButton in the VerticalBox ButtonsBoxName, and adds a
	 * LocalConnectClass child named "LocalConnectWidget" centered on the CanvasPanel ParentPanelName. Names match the
	 * BindWidgets on UGeoMainMenuWidget. Re-run-safe. Compiles and saves the asset.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void AddLocalConnectToMainMenu(UWidgetBlueprint* WidgetBlueprint, FName ParentPanelName, FName ButtonsBoxName,
										  TSubclassOf<UUserWidget> MenuButtonClass,
										  TSubclassOf<UUserWidget> LocalConnectClass);

};
