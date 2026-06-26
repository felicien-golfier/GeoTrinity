// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Tool/GeoWidgetBuilderUtil.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelSlot.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Widget.h"
#include "FileHelpers.h"
#include "Animation/WidgetAnimation.h"
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

	// A rebuild that changes the root's class (e.g. Overlay → VerticalBox) would otherwise fatal in ConstructWidget:
	// the previous root UObject still occupies Name, and UE refuses to replace an existing object with a different class.
	// Rename the stale occupant to a unique transient name so the new root can claim Name.
	if (UObject* Existing = StaticFindObjectFast(UObject::StaticClass(), Tree, Name))
	{
		Existing->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
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
UWidget* UGeoWidgetBuilderUtil::ConstructWidgetInTree(UWidgetBlueprint* WidgetBlueprint, TSubclassOf<UWidget> WidgetClass,
													  FName WidgetName, bool bIsVariable)
{
	if (!ensureMsgf(WidgetClass, TEXT("ConstructWidgetInTree — WidgetClass is null")))
	{
		return nullptr;
	}

	UWidgetTree* Tree = WidgetBlueprint ? WidgetBlueprint->WidgetTree : nullptr;
	if (!ensureMsgf(Tree, TEXT("ConstructWidgetInTree — WidgetTree is null on '%s'"),
					WidgetBlueprint ? *WidgetBlueprint->GetName() : TEXT("null")))
	{
		return nullptr;
	}

	// Reuse-safe: drop any existing widget of this name (parented or not) so a re-run rebuilds cleanly. Renaming the
	// stale occupant out of the tree frees the name — ConstructWidget would otherwise collide on the same Outer+name.
	if (UWidget* Existing = FindTreeWidget(Tree, WidgetName))
	{
		Existing->RemoveFromParent();
		Existing->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
	}

	Tree->Modify();
	WidgetBlueprint->Modify();

	UWidget* Widget = Tree->ConstructWidget<UWidget>(WidgetClass, WidgetName);
	Widget->bIsVariable = bIsVariable;
	// GUID registration is deferred to CommitTree, which reconciles the whole map against the tree. Registering here
	// would leak a dangling GUID if the batch aborts before commit (the compiler then fails to load with "Variable was
	// deleted but still has a GUID").
	return Widget;
}

// ---------------------------------------------------------------------------------------------------------------------
UPanelSlot* UGeoWidgetBuilderUtil::AttachWidget(UWidgetBlueprint* WidgetBlueprint, FName ParentName, FName ChildName,
												int32 Index)
{
	UWidgetTree* Tree = WidgetBlueprint ? WidgetBlueprint->WidgetTree : nullptr;
	if (!ensureMsgf(Tree, TEXT("AttachWidget — WidgetTree is null on '%s'"),
					WidgetBlueprint ? *WidgetBlueprint->GetName() : TEXT("null")))
	{
		return nullptr;
	}

	UPanelWidget* Parent = Cast<UPanelWidget>(FindTreeWidget(Tree, ParentName));
	UWidget* Child = FindTreeWidget(Tree, ChildName);
	if (!ensureMsgf(Parent, TEXT("AttachWidget — no panel named '%s' in '%s'"), *ParentName.ToString(),
					*WidgetBlueprint->GetName())
		|| !ensureMsgf(Child, TEXT("AttachWidget — no widget named '%s' in '%s'"), *ChildName.ToString(),
					   *WidgetBlueprint->GetName()))
	{
		return nullptr;
	}

	Tree->Modify();
	WidgetBlueprint->Modify();

	// Detach first so re-parenting (and re-running) is clean; the widget object — name, GUID, graph bindings — survives.
	Child->RemoveFromParent();

	// InsertChildAt clamps the index; -1 (or out of range) appends.
	return Index >= 0 ? Parent->InsertChildAt(Index, Child) : Parent->AddChild(Child);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoWidgetBuilderUtil::RemoveWidget(UWidgetBlueprint* WidgetBlueprint, FName Name)
{
	UWidgetTree* Tree = WidgetBlueprint ? WidgetBlueprint->WidgetTree : nullptr;
	if (!ensureMsgf(Tree, TEXT("RemoveWidget — WidgetTree is null on '%s'"),
					WidgetBlueprint ? *WidgetBlueprint->GetName() : TEXT("null")))
	{
		return;
	}

	UWidget* Widget = FindTreeWidget(Tree, Name);
	if (!ensureMsgf(Widget, TEXT("RemoveWidget — no widget named '%s' in '%s'"), *Name.ToString(),
					*WidgetBlueprint->GetName()))
	{
		return;
	}

	Tree->Modify();
	WidgetBlueprint->Modify();
	Widget->RemoveFromParent();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoWidgetBuilderUtil::CommitTree(UWidgetBlueprint* WidgetBlueprint)
{
	if (!ensureMsgf(WidgetBlueprint, TEXT("CommitTree — WidgetBlueprint is null")))
	{
		return;
	}

	// Reconcile the variable-name → GUID map against the tree before compiling, mirroring the widget-BP compiler's own
	// ValidateAndFixUpVariableGuids: it requires EVERY tree widget (not just bIsVariable ones — the root and any named
	// widget count too) to have a GUID, and prunes entries whose widget is gone. Matching that set here keeps a batch of
	// Construct/Attach/Remove ops consistent so the compiler's verify pass does not trip ("did not get a GUID" /
	// "was deleted but still has a GUID"). GetAllWidgets returns the root plus all descendants — the compiler's source
	// set. Animation GUIDs (kept in Animations) are intentionally left to the compiler and never pruned here.
	if (UWidgetTree* Tree = WidgetBlueprint->WidgetTree)
	{
		TArray<UWidget*> AllWidgets;
		Tree->GetAllWidgets(AllWidgets);

		TSet<FName> WidgetNames;
		for (UWidget* Widget : AllWidgets)
		{
			if (Widget)
			{
				WidgetNames.Add(Widget->GetFName());
				if (!WidgetBlueprint->WidgetVariableNameToGuidMap.Contains(Widget->GetFName()))
				{
					WidgetBlueprint->WidgetVariableNameToGuidMap.Add(Widget->GetFName(), FGuid::NewGuid());
				}
			}
		}

		for (auto It = WidgetBlueprint->WidgetVariableNameToGuidMap.CreateIterator(); It; ++It)
		{
			bool const bIsAnimation = WidgetBlueprint->Animations.ContainsByPredicate(
				[&It](UWidgetAnimation const* Animation)
				{
					return Animation && Animation->GetFName() == It.Key();
				});
			if (!WidgetNames.Contains(It.Key()) && !bIsAnimation)
			{
				It.RemoveCurrent();
			}
		}
	}

	FinishBuild(WidgetBlueprint);
}

// ---------------------------------------------------------------------------------------------------------------------
UWidget* UGeoWidgetBuilderUtil::AddWidgetToPanel(UWidgetBlueprint* WidgetBlueprint, FName ParentPanelName,
												 TSubclassOf<UWidget> WidgetClass, FName WidgetName, FMargin Offsets)
{
	UWidget* Widget = ConstructWidgetInTree(WidgetBlueprint, WidgetClass, WidgetName, /*bIsVariable*/ true);
	if (!Widget)
	{
		return nullptr;
	}

	UPanelSlot* Slot = AttachWidget(WidgetBlueprint, ParentPanelName, WidgetName);

	// CanvasPanel placement is purely numeric (anchors + pixel offsets), so set it here as a convenience. Other panels
	// (Overlay, VerticalBox, …) carry alignment/fill semantics the caller positions afterward via the returned widget.
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
	{
		CanvasSlot->SetAnchors(FAnchors(0.f, 0.f));
		CanvasSlot->SetOffsets(Offsets);
		CanvasSlot->SetAutoSize(false);
	}

	CommitTree(WidgetBlueprint);

	UE_LOG(LogTemp, Log, TEXT("GeoWidgetBuilderUtil: Added %s '%s' under '%s' on '%s'"), *WidgetClass->GetName(),
		   *WidgetName.ToString(), *ParentPanelName.ToString(), *WidgetBlueprint->GetName());
	return Widget;
}

// ---------------------------------------------------------------------------------------------------------------------
UPanelWidget* UGeoWidgetBuilderUtil::GroupWidgetsIntoPanel(UWidgetBlueprint* WidgetBlueprint, FName ParentPanelName,
														   FName GroupName, TSubclassOf<UPanelWidget> GroupPanelClass,
														   TArray<FName> ChildNames, FMargin GroupOffsets)
{
	UWidget* GroupWidget = ConstructWidgetInTree(WidgetBlueprint, GroupPanelClass, GroupName, /*bIsVariable*/ true);
	UPanelWidget* Group = Cast<UPanelWidget>(GroupWidget);
	if (!Group)
	{
		return nullptr;
	}

	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(AttachWidget(WidgetBlueprint, ParentPanelName, GroupName)))
	{
		CanvasSlot->SetAnchors(FAnchors(0.f, 0.f));
		CanvasSlot->SetOffsets(GroupOffsets);
		CanvasSlot->SetAutoSize(false);
	}

	// Re-parent in the given order so z-order (and thus draw order) is preserved.
	for (FName const ChildName : ChildNames)
	{
		AttachWidget(WidgetBlueprint, GroupName, ChildName);
	}

	CommitTree(WidgetBlueprint);

	UE_LOG(LogTemp, Log, TEXT("GeoWidgetBuilderUtil: Grouped %d widget(s) into %s '%s' under '%s' on '%s'"),
		   ChildNames.Num(), *GroupPanelClass->GetName(), *GroupName.ToString(), *ParentPanelName.ToString(),
		   *WidgetBlueprint->GetName());
	return Group;
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
UVerticalBoxSlot* UGeoWidgetBuilderUtil::AddCenteredChildToVerticalBox(UVerticalBox* VerticalBox, UWidget* Child,
																	   FMargin Padding)
{
	if (!ensureMsgf(VerticalBox && Child, TEXT("UGeoWidgetBuilderUtil::AddCenteredChildToVerticalBox — null arg")))
	{
		return nullptr;
	}

	UVerticalBoxSlot* Slot = VerticalBox->AddChildToVerticalBox(Child);
	if (Slot)
	{
		Slot->SetHorizontalAlignment(HAlign_Center);
		Slot->SetPadding(Padding);
	}
	return Slot;
}

// ---------------------------------------------------------------------------------------------------------------------
UButton* UGeoWidgetBuilderUtil::ConstructLabeledButton(UWidgetTree* Tree, FName Name, FText LabelText)
{
	if (!ensureMsgf(Tree, TEXT("UGeoWidgetBuilderUtil::ConstructLabeledButton — Tree is null")))
	{
		return nullptr;
	}

	UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
	UTextBlock* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(*(Name.ToString() + TEXT("Label"))));
	Label->SetText(LabelText);
	Button->SetContent(Label);
	return Button;
}

// ---------------------------------------------------------------------------------------------------------------------
UProgressBar* UGeoWidgetBuilderUtil::ConstructProgressBar(UWidgetTree* Tree, FName Name, FLinearColor FillColor,
														  FLinearColor BackgroundColor, bool bIsVariable)
{
	if (!ensureMsgf(Tree, TEXT("UGeoWidgetBuilderUtil::ConstructProgressBar — Tree is null")))
	{
		return nullptr;
	}

	UProgressBar* Bar = Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), Name);
	Bar->bIsVariable = bIsVariable;
	FProgressBarStyle Style = Bar->GetWidgetStyle();
	Style.BackgroundImage.TintColor = FSlateColor(BackgroundColor);
	Style.FillImage.TintColor = FSlateColor(FillColor);
	Bar->SetWidgetStyle(Style);
	Bar->SetFillColorAndOpacity(FLinearColor::White);
	Bar->SetPercent(0.f);
	return Bar;
}

// ---------------------------------------------------------------------------------------------------------------------
UOverlaySlot* UGeoWidgetBuilderUtil::AddFillChildToOverlay(UOverlay* Overlay, UWidget* Child)
{
	if (!ensureMsgf(Overlay && Child, TEXT("UGeoWidgetBuilderUtil::AddFillChildToOverlay — Overlay or Child is null")))
	{
		return nullptr;
	}

	UOverlaySlot* Slot = Cast<UOverlaySlot>(Overlay->AddChildToOverlay(Child));
	if (Slot)
	{
		Slot->SetHorizontalAlignment(HAlign_Fill);
		Slot->SetVerticalAlignment(VAlign_Fill);
	}
	return Slot;
}

// ---------------------------------------------------------------------------------------------------------------------
UWidget* UGeoWidgetBuilderUtil::FindTreeWidget(UWidgetTree* Tree, FName Name)
{
	if (!Tree)
	{
		return nullptr;
	}

	// FindWidget only walks from the root, so a just-constructed, not-yet-parented widget is invisible to it. Fall back
	// to an outer-scoped object find: ConstructWidget makes the widget an Outer-child of the tree, so this catches it
	// whether or not it is parented.
	if (UWidget* Found = Tree->FindWidget(Name))
	{
		return Found;
	}
	return Cast<UWidget>(StaticFindObjectFast(UWidget::StaticClass(), Tree, Name));
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
