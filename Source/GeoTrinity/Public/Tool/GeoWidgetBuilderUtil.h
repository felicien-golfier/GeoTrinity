// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
#include "EditorUtilityObject.h"

#include "GeoWidgetBuilderUtil.generated.h"

class UButton;
class UCanvasPanelSlot;
class UMaterialInterface;
class UOverlay;
class UOverlaySlot;
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
class GEOTRINITY_API UGeoWidgetBuilderUtil : public UEditorUtilityObject
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
};

#endif // WITH_EDITOR
