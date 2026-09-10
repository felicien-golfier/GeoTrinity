// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityObject.h"
#include "Fonts/SlateFontInfo.h"

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
	 * Builds the WBP_StatusBar tree: an empty CanvasPanel root. UGeoStatusBarWidget builds its own icon-row tree
	 * entirely in C++ (UGeoStatusBarWidget::Initialize) and only needs a compilable placeholder root here so the
	 * asset exists as a Blueprint the designer can reparent-configure (e.g. tweak DepletionSweepMaterial). Compiles
	 * and saves the asset.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void BuildStatusBarWidget(UWidgetBlueprint* WidgetBlueprint);

	/**
	 * Adds StatusBarClass into the overlay's CanvasPanel ParentPanelName as a child named "StatusBar" (matching the
	 * BindWidget on UGeoOverlayWidget), anchored bottom-center, sitting BottomFraction above AbilityBar's own bottom
	 * offset so the row reads above the ability bar. Sizing/positioning is expressed as fractions of the canvas so it
	 * holds the same screen proportion at any resolution. Composes UGeoWidgetBuilderUtil::AddChildToCanvasPanel;
	 * compiles and saves the asset.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void AddStatusBarToOverlay(UWidgetBlueprint* WidgetBlueprint, FName ParentPanelName,
									  TSubclassOf<UUserWidget> StatusBarClass, float WidthFraction = 0.4f,
									  float HeightFraction = 0.06f, float BottomFraction = 0.1f);

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
	 * Builds the WBP_Leaderboard tree (the recorded-fights panel). The panel owns no look: its whole tree is one
	 * ListPanelClass instance named "ListFrame" (the shared list frame, matching the BindWidget on
	 * UGeoListPanelWidget), with a HeaderBox holding the title and the TabsBox strip dropped into its header slot and
	 * BackButton into its footer slot. TabsBox and BackButton match the BindWidgets on UGeoLeaderboardWidget, which
	 * fills the strip with one tab per boss and the frame's scroll box with one row per recorded attempt at runtime.
	 * The title wears TitleFont, so a built header carries the same typography as the authored menus.
	 * Compiles and saves the asset.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void BuildLeaderboardWidget(UWidgetBlueprint* WidgetBlueprint, TSubclassOf<UUserWidget> ListPanelClass,
									   TSubclassOf<UUserWidget> MenuButtonClass, FSlateFontInfo TitleFont);

	/**
	 * Builds the shared list-frame tree (WBP_ListPanel): an Overlay root filling the screen, holding a VerticalBox of
	 *   HeaderArea (auto height: HeaderBackground under the HeaderSlot the panel drops its controls into)
	 *   ContentArea (the rest: ContentBackground under RowsBox, with FooterSlot in the bottom-right corner).
	 * RowsBox matches the BindWidget on UGeoListFrameWidget; the two named slots are what each panel fills with its
	 * own header controls and back button. Every list panel instantiates this one asset, so the two background images
	 * are the skin of all of them — the caller keeps their brushes across a rebuild by reading them off first and
	 * writing them back after. Compiles and saves the asset.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void BuildListPanelWidget(UWidgetBlueprint* WidgetBlueprint);

	/**
	 * Builds the shared list-row tree (WBP_ListRow): a UGeoButton root "RowButton" holding the HorizontalBox
	 * "ColumnsBox", both matching the BindWidgets on UGeoListRowWidget, which fills the box with the columns of
	 * whatever list built the row. Every list in the game instantiates this one asset, so the button style set on it
	 * is the skin of all of them — the caller keeps that style across a rebuild by reading it off RowButton first and
	 * writing it back after. Compiles and saves the asset.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void BuildListRowWidget(UWidgetBlueprint* WidgetBlueprint);

	/**
	 * Appends one entry to the existing main-menu tree WITHOUT rebuilding it: inserts a MenuButtonClass button named
	 * ButtonName carrying ButtonLabel, plus its spacer, just above QuitButton in the VerticalBox ButtonsBoxName, and
	 * adds a PanelClass child named PanelName centered on the CanvasPanel ParentPanelName. Both names must match the
	 * BindWidgets on UGeoMainMenuWidget ("PlayLocalButton"/"LocalConnectWidget", "LeaderboardButton"/
	 * "LeaderboardWidget", …). Re-run-safe. Compiles and saves the asset.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void AddPanelEntryToMainMenu(UWidgetBlueprint* WidgetBlueprint, FName ParentPanelName, FName ButtonsBoxName,
										TSubclassOf<UUserWidget> MenuButtonClass, FName ButtonName, FText ButtonLabel,
										FName PanelName, TSubclassOf<UUserWidget> PanelClass);
};
