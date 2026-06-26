// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityObject.h"

#include "GeoWidgetBuilderUtil.generated.h"

class UButton;
class UCanvasPanelSlot;
class UMaterialInterface;
class UOverlay;
class UOverlaySlot;
class UPanelSlot;
class UPanelWidget;
class UProgressBar;
class UUserWidget;
class UVerticalBox;
class UVerticalBoxSlot;
class UWidgetBlueprint;
class UWidget;
class UWidgetTree;

/**
 * Generic, reusable widget-tree primitives for Python/Blueprint automation. Content-specific builders (per-widget
 * trees) live in UGeoHudWidgetBuilderUtil and compose these. Keep this class free of one-off, per-asset functions.
 */
UCLASS()
class GEOTRINITYEDITOR_API UGeoWidgetBuilderUtil : public UEditorUtilityObject
{
	GENERATED_BODY()

public:
	/**
	 * Generic: replaces the widget's root with a freshly constructed panel of PanelClass (CanvasPanel, Overlay,
	 * HorizontalBox, …), named RootName. Pass a BindWidget variable name (e.g. "SlotBox") so the C++ widget can bind
	 * the root. The panel is left empty for runtime population or further building. Compiles and saves the asset.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void SetRootPanel(UWidgetBlueprint* WidgetBlueprint, TSubclassOf<UPanelWidget> PanelClass,
							 FName RootName = TEXT("Root"));

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

	// --- Low-level tree primitives ----------------------------------------------------------------------------------
	// These four expose the only widget-tree operations Python cannot do itself (the WidgetTree property is protected):
	// construct, (re)attach, remove, and persist. Everything else — every set_text/set_brush/set_padding/set_anchors/… —
	// is already reachable on the returned widget/slot objects from Python. Compose these from Python to build, move,
	// wrap, reorder, or delete any widget WITHOUT a recompile. They do not compile/save individually; batch your edits
	// then call CommitTree once. The higher-level helpers below (AddWidgetToPanel, GroupWidgetsIntoPanel) are convenience
	// wrappers over these for common one-call cases.

	/**
	 * Constructs a widget of WidgetClass named WidgetName in the tree and returns it (unparented). When bIsVariable, flags
	 * it as a graph variable and registers a fresh GUID so the compiler's verify pass accepts it. Reuse-safe: removes any
	 * existing widget of that name first. Does NOT attach or save — call AttachWidget then CommitTree. Null on failure.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static UWidget* ConstructWidgetInTree(UWidgetBlueprint* WidgetBlueprint, TSubclassOf<UWidget> WidgetClass,
										  FName WidgetName, bool bIsVariable = true);

	/**
	 * Attaches the existing widget ChildName into the panel ParentName, optionally at Index (-1 = append). If the child is
	 * already parented it is detached first (re-parent), keeping its name/GUID/graph bindings intact. Returns the new
	 * UPanelSlot for the caller to position from Python (cast to the concrete slot type and call its set_* methods). Does
	 * NOT save — call CommitTree after a batch. Null on failure.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static UPanelSlot* AttachWidget(UWidgetBlueprint* WidgetBlueprint, FName ParentName, FName ChildName,
									int32 Index = -1);

	/** Removes the widget Name from the tree (detaches from its parent). Does NOT save — call CommitTree after. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void RemoveWidget(UWidgetBlueprint* WidgetBlueprint, FName Name);

	/** Compiles and saves the widget blueprint. Call once after a batch of Construct/Attach/Remove + Python slot edits. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void CommitTree(UWidgetBlueprint* WidgetBlueprint);

	// --- Convenience wrappers over the primitives -------------------------------------------------------------------

	/**
	 * Adds a freshly constructed widget of WidgetClass (any UWidget: TextBlock, Image, Border, …) named WidgetName into
	 * the panel ParentPanelName (any UPanelWidget: CanvasPanel, Overlay, VerticalBox, …) in an existing tree, WITHOUT
	 * rebuilding it. WidgetName becomes a BindWidget/graph variable, so a fresh GUID is registered for it. Reuse-safe: any
	 * existing widget of that name is removed first. Layout is left at the panel's slot defaults — the caller positions it
	 * afterward via the returned widget's Slot (e.g. a CanvasPanelSlot's offsets, or an OverlaySlot's alignment); on a
	 * CanvasPanel pass Offsets to also set a top-left-anchored pixel rect in one call (ignored for non-canvas panels).
	 * Returns the constructed widget (null on failure). Compiles and saves the asset. The BP graph drives the new widget.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static UWidget* AddWidgetToPanel(UWidgetBlueprint* WidgetBlueprint, FName ParentPanelName,
									 TSubclassOf<UWidget> WidgetClass, FName WidgetName, FMargin Offsets);

	/**
	 * Wraps a set of EXISTING widgets (by name) into a freshly constructed panel of GroupPanelClass (any UPanelWidget:
	 * Overlay, VerticalBox, …) named GroupName, inserted under ParentPanelName at top-left-anchored pixel GroupOffsets
	 * (applied when ParentPanelName is a CanvasPanel). The moved children KEEP their names, GUIDs, and graph bindings — they
	 * are re-parented, not re-created — so existing event-graph wiring on them survives. Grouping under one anchored panel
	 * lets the whole cluster scale/move together. The caller positions each moved child inside the group afterward via its
	 * Slot (e.g. OverlaySlot padding/alignment). GroupName becomes a graph variable (fresh GUID registered). Reuse-safe:
	 * children already inside an existing GroupName are pulled back out and re-grouped. Returns the group panel (null on
	 * failure). Compiles and saves the asset.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static UPanelWidget* GroupWidgetsIntoPanel(UWidgetBlueprint* WidgetBlueprint, FName ParentPanelName, FName GroupName,
											   TSubclassOf<UPanelWidget> GroupPanelClass, TArray<FName> ChildNames,
											   FMargin GroupOffsets);

	// --- Building blocks for content builders (UGeoHudWidgetBuilderUtil and future per-widget builders) ---

	/** Validates the blueprint and its tree, marks both for modification, and clears the root. Returns null on failure. */
	static UWidgetTree* BeginBuild(UWidgetBlueprint* WidgetBlueprint, TCHAR const* FunctionName);

	/** Compiles and saves the widget blueprint after its tree has been built. */
	static void FinishBuild(UWidgetBlueprint* WidgetBlueprint);

	/** Constructs a panel of PanelClass named Name and assigns it as the tree root. Returns it (null on failure). */
	static UPanelWidget* ConstructRootPanel(UWidgetTree* Tree, TSubclassOf<UPanelWidget> PanelClass, FName Name);

	/**
	 * Adds an instance of ChildWidgetClass (a UserWidget) named ChildName under the CanvasPanel ParentPanelName in an
	 * existing tree, WITHOUT clearing it. ChildName becomes a BindWidget variable, so a fresh GUID is registered for it.
	 * Returns the new canvas slot for the caller to position (the content builder owns layout policy); null on failure.
	 * Does not compile/save — the caller does that via FinishBuild after positioning.
	 */
	static UCanvasPanelSlot* AddChildToCanvasPanel(UWidgetBlueprint* WidgetBlueprint, FName ParentPanelName,
												   TSubclassOf<UUserWidget> ChildWidgetClass, FName ChildName);

	/** Adds Child to VerticalBox horizontally centered with Padding, and returns its slot (null on failure). */
	static UVerticalBoxSlot* AddCenteredChildToVerticalBox(UVerticalBox* VerticalBox, UWidget* Child, FMargin Padding);

	/** Constructs a Button named Name in Tree whose content is a TextBlock showing LabelText. Returns it (null on failure). */
	static UButton* ConstructLabeledButton(UWidgetTree* Tree, FName Name, FText LabelText);

	/**
	 * Constructs a UProgressBar named Name in Tree with the given fill/background tints, starting empty (LeftToRight fill;
	 * callers needing another fill type set it on the returned bar). Set bIsVariable when the C++ widget BindWidgets it.
	 * Returns it (null on failure).
	 */
	static UProgressBar* ConstructProgressBar(UWidgetTree* Tree, FName Name, FLinearColor FillColor,
											  FLinearColor BackgroundColor, bool bIsVariable = false);

	/** Adds Child to Overlay as a fill/fill slot (covers the whole overlay rect). Returns its slot (null on failure). */
	static UOverlaySlot* AddFillChildToOverlay(UOverlay* Overlay, UWidget* Child);

private:
	static void LogWidget(UWidget* Widget, int32 Depth);

	/**
	 * Finds a tree-owned widget by name, INCLUDING a freshly constructed one not yet parented. UWidgetTree::FindWidget
	 * only walks from the root, so it misses unparented widgets (e.g. the output of ConstructWidgetInTree before its
	 * first AttachWidget). ConstructWidget makes the widget an Outer-child of the tree, so an outer-scoped object find
	 * locates it regardless of parenting.
	 */
	static UWidget* FindTreeWidget(UWidgetTree* Tree, FName Name);
};
