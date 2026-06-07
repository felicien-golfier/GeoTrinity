// Copyright 2024 GeoTrinity. All Rights Reserved.

#if WITH_EDITOR

#include "Tool/GeoWidgetBuilderUtil.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/Widget.h"
#include "FileHelpers.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Materials/MaterialInterface.h"
#include "WidgetBlueprint.h"

// ---------------------------------------------------------------------------------------------------------------------
UWidgetTree* UGeoWidgetBuilderUtil::BeginBuild(UWidgetBlueprint* WidgetBlueprint, TCHAR const* FunctionName)
{
	if (!ensureMsgf(WidgetBlueprint, TEXT("UGeoWidgetBuilderUtil::%s — WidgetBlueprint is null"), FunctionName))
	{
		return nullptr;
	}

	UWidgetTree* Tree = WidgetBlueprint->WidgetTree;
	if (!ensureMsgf(Tree, TEXT("UGeoWidgetBuilderUtil::%s — WidgetTree is null on '%s'"), FunctionName,
					*WidgetBlueprint->GetName()))
	{
		return nullptr;
	}

	Tree->Modify();
	WidgetBlueprint->Modify();

	// Clear any existing root before rebuilding
	Tree->RootWidget = nullptr;

	// Clear the widget-variable GUID map too. The compiler only auto-assigns deterministic GUIDs when this map is
	// empty (see UWidgetBlueprintCompilerContext); on a rebuild of an existing asset the stale entries (named for the
	// PREVIOUS tree) skip that path, so freshly constructed widgets get no GUID and the compiler's verify pass trips
	// "Widget [X] was added but did not get a GUID". Emptying it lets the compiler repopulate cleanly for the new tree.
	WidgetBlueprint->WidgetVariableNameToGuidMap.Empty();

	return Tree;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoWidgetBuilderUtil::FinishBuild(UWidgetBlueprint* WidgetBlueprint)
{
	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
	UEditorLoadingAndSavingUtils::SavePackages({WidgetBlueprint->GetPackage()}, false);
}

// ---------------------------------------------------------------------------------------------------------------------
UPanelWidget* UGeoWidgetBuilderUtil::ConstructRootPanel(UWidgetTree* Tree, TSubclassOf<UPanelWidget> PanelClass,
														FName Name)
{
	if (!ensureMsgf(PanelClass, TEXT("UGeoWidgetBuilderUtil::ConstructRootPanel — PanelClass is null")))
	{
		return nullptr;
	}

	UPanelWidget* Panel = Tree->ConstructWidget<UPanelWidget>(PanelClass, Name);
	Tree->RootWidget = Panel;
	return Panel;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoWidgetBuilderUtil::SetRootPanel(UWidgetBlueprint* WidgetBlueprint, TSubclassOf<UPanelWidget> PanelClass,
										 FName RootName)
{
	UWidgetTree* Tree = BeginBuild(WidgetBlueprint, TEXT("SetRootPanel"));
	if (!Tree || !ConstructRootPanel(Tree, PanelClass, RootName))
	{
		return;
	}

	FinishBuild(WidgetBlueprint);

	UE_LOG(LogTemp, Log, TEXT("GeoWidgetBuilderUtil: Set root panel '%s' (named '%s') on '%s'"), *PanelClass->GetName(),
		   *RootName.ToString(), *WidgetBlueprint->GetName());
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoWidgetBuilderUtil::SetImageRoot(UWidgetBlueprint* WidgetBlueprint, UTexture2D* Texture, FVector2D DesiredSize)
{
	if (!ensureMsgf(Texture, TEXT("UGeoWidgetBuilderUtil::SetImageRoot — Texture is null")))
	{
		return;
	}

	UWidgetTree* Tree = BeginBuild(WidgetBlueprint, TEXT("SetImageRoot"));
	if (!Tree)
	{
		return;
	}

	UImage* Image = Tree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image"));
	Image->SetBrushFromTexture(Texture);
	Image->SetDesiredSizeOverride(DesiredSize);
	Tree->RootWidget = Image;

	FinishBuild(WidgetBlueprint);

	UE_LOG(LogTemp, Log, TEXT("GeoWidgetBuilderUtil: Set image root on '%s' with texture '%s' (%gx%g)"),
		   *WidgetBlueprint->GetName(), *Texture->GetName(), DesiredSize.X, DesiredSize.Y);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoWidgetBuilderUtil::SetImageRootFromMaterial(UWidgetBlueprint* WidgetBlueprint, UMaterialInterface* Material,
													 FVector2D DesiredSize)
{
	if (!ensureMsgf(Material, TEXT("UGeoWidgetBuilderUtil::SetImageRootFromMaterial — Material is null")))
	{
		return;
	}

	UWidgetTree* Tree = BeginBuild(WidgetBlueprint, TEXT("SetImageRootFromMaterial"));
	if (!Tree)
	{
		return;
	}

	UImage* Image = Tree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image"));
	Image->SetBrushFromMaterial(Material);
	Image->SetDesiredSizeOverride(DesiredSize);
	Tree->RootWidget = Image;

	FinishBuild(WidgetBlueprint);

	UE_LOG(LogTemp, Log, TEXT("GeoWidgetBuilderUtil: Set image root on '%s' with material '%s' (%gx%g)"),
		   *WidgetBlueprint->GetName(), *Material->GetName(), DesiredSize.X, DesiredSize.Y);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoWidgetBuilderUtil::InspectWidgetBlueprint(UWidgetBlueprint* WidgetBlueprint)
{
	if (!ensureMsgf(WidgetBlueprint, TEXT("UGeoWidgetBuilderUtil::InspectWidgetBlueprint — WidgetBlueprint is null")))
	{
		return;
	}

	UWidgetTree* Tree = WidgetBlueprint->WidgetTree;
	if (!ensureMsgf(Tree, TEXT("UGeoWidgetBuilderUtil::InspectWidgetBlueprint — WidgetTree is null on '%s'"),
					*WidgetBlueprint->GetName()))
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("=== InspectWidgetBlueprint: %s ==="), *WidgetBlueprint->GetName());

	if (!Tree->RootWidget)
	{
		UE_LOG(LogTemp, Log, TEXT("  (empty — no root widget)"));
		return;
	}

	LogWidget(Tree->RootWidget, 0);
}

// ---------------------------------------------------------------------------------------------------------------------
UCanvasPanelSlot* UGeoWidgetBuilderUtil::AddChildToCanvasPanel(UWidgetBlueprint* WidgetBlueprint, FName ParentPanelName,
															   TSubclassOf<UUserWidget> ChildWidgetClass, FName ChildName)
{
	if (!ensureMsgf(WidgetBlueprint, TEXT("UGeoWidgetBuilderUtil::AddChildToCanvasPanel — WidgetBlueprint is null")) ||
		!ensureMsgf(ChildWidgetClass, TEXT("UGeoWidgetBuilderUtil::AddChildToCanvasPanel — ChildWidgetClass is null")))
	{
		return nullptr;
	}

	UWidgetTree* Tree = WidgetBlueprint->WidgetTree;
	if (!ensureMsgf(Tree, TEXT("UGeoWidgetBuilderUtil::AddChildToCanvasPanel — WidgetTree is null on '%s'"),
					*WidgetBlueprint->GetName()))
	{
		return nullptr;
	}

	// Unlike the root builders, this APPENDS to the existing tree: do NOT clear RootWidget or the GUID map. Existing
	// widgets keep their GUIDs; only the new child's variable name needs a fresh GUID registered (the compiler auto-
	// assigns GUIDs only when WidgetVariableNameToGuidMap is empty, which it is not on an already-built asset).
	UCanvasPanel* Parent = Cast<UCanvasPanel>(Tree->FindWidget(ParentPanelName));
	if (!ensureMsgf(Parent, TEXT("AddChildToCanvasPanel — no CanvasPanel named '%s' in '%s'"),
					*ParentPanelName.ToString(), *WidgetBlueprint->GetName()))
	{
		return nullptr;
	}

	// Reuse-safe: drop any existing child of this name so a re-run rebuilds cleanly instead of duplicating.
	if (UWidget* Existing = Tree->FindWidget(ChildName))
	{
		Existing->RemoveFromParent();
	}

	Tree->Modify();
	WidgetBlueprint->Modify();

	UUserWidget* Child = Tree->ConstructWidget<UUserWidget>(ChildWidgetClass, ChildName);
	UCanvasPanelSlot* ChildSlot = Parent->AddChildToCanvas(Child);

	// Register a GUID for the new BindWidget variable so the compiler's verify pass ("Widget [X] did not get a GUID")
	// passes; with a non-empty map the compiler will not mint one itself.
	WidgetBlueprint->WidgetVariableNameToGuidMap.Add(ChildName, FGuid::NewGuid());

	return ChildSlot;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoWidgetBuilderUtil::LogWidget(UWidget* Widget, int32 Depth)
{
	if (!Widget)
	{
		return;
	}

	FString Indent = FString::ChrN(Depth * 2, ' ');

	// --- Slot info (if this widget is slotted inside a parent) ---
	FString SlotInfo;
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot))
	{
		FAnchors A = CanvasSlot->GetAnchors();
		FMargin O = CanvasSlot->GetOffsets();
		FVector2D Al = CanvasSlot->GetAlignment();
		bool AS = CanvasSlot->GetAutoSize();
		SlotInfo = FString::Printf(
			TEXT("Anchors(%.2f,%.2f,%.2f,%.2f) Offsets(L=%.1f T=%.1f W=%.1f H=%.1f) Align(%.2f,%.2f) AutoSize=%s"),
			A.Minimum.X, A.Minimum.Y, A.Maximum.X, A.Maximum.Y, O.Left, O.Top, O.Right, O.Bottom, Al.X, Al.Y,
			AS ? TEXT("true") : TEXT("false"));
	}

	// --- Widget-type-specific properties ---
	FString WidgetInfo;
	if (UProgressBar* Bar = Cast<UProgressBar>(Widget))
	{
		FProgressBarStyle const& Style = Bar->GetWidgetStyle();
		FLinearColor BgColor = Style.BackgroundImage.TintColor.GetSpecifiedColor();
		FLinearColor FillColor = Style.FillImage.TintColor.GetSpecifiedColor();
		int32 FillType = Bar->GetBarFillType();
		WidgetInfo = FString::Printf(TEXT("Percent=%.2f FillType=%d Bg(%.2f,%.2f,%.2f,%.2f) Fill(%.2f,%.2f,%.2f,%.2f)"),
									 Bar->GetPercent(), FillType, BgColor.R, BgColor.G, BgColor.B, BgColor.A,
									 FillColor.R, FillColor.G, FillColor.B, FillColor.A);
	}

	UE_LOG(LogTemp, Log, TEXT("%s[%s] '%s'%s%s"), *Indent, *Widget->GetClass()->GetName(), *Widget->GetName(),
		   SlotInfo.IsEmpty() ? TEXT("") : *(TEXT(" | Slot: ") + SlotInfo),
		   WidgetInfo.IsEmpty() ? TEXT("") : *(TEXT(" | ") + WidgetInfo));

	// --- Recurse into children ---
	if (UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
	{
		for (int32 i = 0; i < Panel->GetChildrenCount(); ++i)
		{
			LogWidget(Panel->GetChildAt(i), Depth + 1);
		}
	}
}

#endif // WITH_EDITOR
