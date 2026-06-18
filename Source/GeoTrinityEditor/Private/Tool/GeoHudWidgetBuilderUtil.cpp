// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Tool/GeoHudWidgetBuilderUtil.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "HUD/Menu/GeoMenuButton.h"
#include "Tool/GeoWidgetBuilderUtil.h"
#include "WidgetBlueprint.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoHudWidgetBuilderUtil::BuildAbilitySlotWidget(UWidgetBlueprint* WidgetBlueprint, float SquareSize,
													  EAbilitySlotKeyLabelPlacement KeyLabelPlacement)
{
	UWidgetTree* Tree = UGeoWidgetBuilderUtil::BeginBuild(WidgetBlueprint, TEXT("BuildAbilitySlotWidget"));
	if (!Tree)
	{
		return;
	}

	// Square — a SizeBox with an explicit SquareSize x SquareSize so the slot has a stable size no matter what the icon or
	// cooldown material reports as its desired size (otherwise an empty/placeholder icon collapses the slot and it grows
	// only once the cooldown sweep renders). Resolution scaling is the bar's job (fractional anchors + UMG DPI).
	USizeBox* Square = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Square"));
	Square->SetWidthOverride(SquareSize);
	Square->SetHeightOverride(SquareSize);

	// The root depends on where the key label goes: a VerticalBox stacks the square above the label (Below); otherwise an
	// Overlay just wraps the square and the label (if any) is stacked over the icon by the inner Stack.
	if (KeyLabelPlacement == EAbilitySlotKeyLabelPlacement::Below)
	{
		UVerticalBox* Root =
			Cast<UVerticalBox>(UGeoWidgetBuilderUtil::ConstructRootPanel(Tree, UVerticalBox::StaticClass(), TEXT("Root")));
		UGeoWidgetBuilderUtil::AddCenteredChildToVerticalBox(Root, Square, FMargin(0.f));

		// Constrain the key label to the slot width: a SizeBox caps the width at SquareSize and a ScaleBox shrinks the text
		// (ScaleToFitX, DownOnly — never enlarges) so a wide key name like "Left Shift" scales down instead of overflowing.
		USizeBox* KeyWidth = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("KeyWidth"));
		KeyWidth->SetWidthOverride(SquareSize);
		UScaleBox* KeyScale = Tree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("KeyScale"));
		KeyScale->SetStretch(EStretch::ScaleToFitX);
		KeyScale->SetStretchDirection(EStretchDirection::DownOnly);
		KeyWidth->AddChild(KeyScale);

		UTextBlock* KeyText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("KeyText"));
		KeyScale->AddChild(KeyText);
		UGeoWidgetBuilderUtil::AddCenteredChildToVerticalBox(Root, KeyWidth, FMargin(0.f, 2.f, 0.f, 0.f));
	}
	else
	{
		UOverlay* Root =
			Cast<UOverlay>(UGeoWidgetBuilderUtil::ConstructRootPanel(Tree, UOverlay::StaticClass(), TEXT("Root")));
		if (UOverlaySlot* SquareSlot = Cast<UOverlaySlot>(Root->AddChildToOverlay(Square)))
		{
			SquareSlot->SetHorizontalAlignment(HAlign_Center);
			SquareSlot->SetVerticalAlignment(VAlign_Center);
		}
	}

	// Inner overlay stacks the icon, cooldown sweep, and text labels inside the square.
	UOverlay* Stack = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Stack"));
	Square->AddChild(Stack);

	// Icon — fills the square.
	UImage* Icon = Tree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Icon"));
	if (UOverlaySlot* IconSlot = Cast<UOverlaySlot>(Stack->AddChildToOverlay(Icon)))
	{
		IconSlot->SetHorizontalAlignment(HAlign_Fill);
		IconSlot->SetVerticalAlignment(VAlign_Fill);
	}

	// CooldownSweep — same footprint as the icon; its MID (assigned at runtime) draws the radial sweep.
	UImage* CooldownSweep = Tree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("CooldownSweep"));
	if (UOverlaySlot* SweepSlot = Cast<UOverlaySlot>(Stack->AddChildToOverlay(CooldownSweep)))
	{
		SweepSlot->SetHorizontalAlignment(HAlign_Fill);
		SweepSlot->SetVerticalAlignment(VAlign_Fill);
	}

	// CountdownText — centered seconds remaining.
	UTextBlock* CountdownText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CountdownText"));
	if (UOverlaySlot* CountdownSlot = Cast<UOverlaySlot>(Stack->AddChildToOverlay(CountdownText)))
	{
		CountdownSlot->SetHorizontalAlignment(HAlign_Center);
		CountdownSlot->SetVerticalAlignment(VAlign_Center);
	}

	// CountText — remaining-deployable badge in the bottom-right corner.
	UTextBlock* CountText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CountText"));
	if (UOverlaySlot* CountSlot = Cast<UOverlaySlot>(Stack->AddChildToOverlay(CountText)))
	{
		CountSlot->SetHorizontalAlignment(HAlign_Right);
		CountSlot->SetVerticalAlignment(VAlign_Bottom);
	}

	// OverlayBottom places KeyText inside the square, bottom-center over the icon (Below already added it under the square).
	if (KeyLabelPlacement == EAbilitySlotKeyLabelPlacement::OverlayBottom)
	{
		UTextBlock* KeyText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("KeyText"));
		if (UOverlaySlot* KeySlot = Cast<UOverlaySlot>(Stack->AddChildToOverlay(KeyText)))
		{
			KeySlot->SetHorizontalAlignment(HAlign_Center);
			KeySlot->SetVerticalAlignment(VAlign_Bottom);
		}
	}

	UGeoWidgetBuilderUtil::FinishBuild(WidgetBlueprint);

	UE_LOG(LogTemp, Log, TEXT("GeoHudWidgetBuilderUtil: Built WBP_AbilitySlot (square, fills bar slot)"));
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoHudWidgetBuilderUtil::BuildAbilityBarWidget(UWidgetBlueprint* WidgetBlueprint)
{
	UWidgetTree* Tree = UGeoWidgetBuilderUtil::BeginBuild(WidgetBlueprint, TEXT("BuildAbilityBarWidget"));
	if (!Tree)
	{
		return;
	}

	// Root Overlay fills the bar region; the SlotBox is centered inside it (rather than stretched) so the run of slots
	// stays packed and centered in the bar. The HorizontalBox sizes to its slots (added at runtime by UGeoAbilityBarWidget).
	UOverlay* Root = Cast<UOverlay>(UGeoWidgetBuilderUtil::ConstructRootPanel(Tree, UOverlay::StaticClass(), TEXT("Root")));

	UHorizontalBox* SlotBox = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SlotBox"));
	if (UOverlaySlot* SlotBoxSlot = Cast<UOverlaySlot>(Root->AddChildToOverlay(SlotBox)))
	{
		SlotBoxSlot->SetHorizontalAlignment(HAlign_Center);
		SlotBoxSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UGeoWidgetBuilderUtil::FinishBuild(WidgetBlueprint);

	UE_LOG(LogTemp, Log, TEXT("GeoHudWidgetBuilderUtil: Built WBP_AbilityBar (centered SlotBox)"));
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoHudWidgetBuilderUtil::BuildChargeBeamGaugeWidget(UWidgetBlueprint* WidgetBlueprint, float SweetSpotMinRatio,
														  float SweetSpotMaxRatio)
{
	UWidgetTree* Tree = UGeoWidgetBuilderUtil::BeginBuild(WidgetBlueprint, TEXT("BuildChargeBeamGaugeWidget"));
	if (!Tree)
	{
		return;
	}

	// --- Root: Canvas Panel ---
	UCanvasPanel* Canvas =
		Cast<UCanvasPanel>(UGeoWidgetBuilderUtil::ConstructRootPanel(Tree, UCanvasPanel::StaticClass(), TEXT("CanvasRoot")));

	constexpr float BarHeight = 80.f;

	// --- ChargeBar: full-width, dark-blue fill ---
	UProgressBar* ChargeBar = UGeoWidgetBuilderUtil::ConstructProgressBar(
		Tree, TEXT("ChargeBar"), FLinearColor(0.1f, 0.4f, 1.0f, 1.0f), FLinearColor(0.1f, 0.4f, 1.0f, 0.5f));
	ChargeBar->SetBarFillType(EProgressBarFillType::BottomToTop);

	UCanvasPanelSlot* ChargeSlot = Canvas->AddChildToCanvas(ChargeBar);
	ChargeSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	ChargeSlot->SetOffsets(FMargin(0.f, 0.f, 0.f, 0.f));
	ChargeSlot->SetAlignment(FVector2D(0.f, 0.f));

	// --- SweetSpotBar: narrow golden overlay at the sweet-spot window ---
	UProgressBar* SweetSpotBar = UGeoWidgetBuilderUtil::ConstructProgressBar(
		Tree, TEXT("SweetSpotBar"), FLinearColor(1.0f, 0.75f, 0.0f, 1.0f), FLinearColor(1.f, 0.75f, 0.0f, 0.5f));
	SweetSpotBar->SetBarFillType(EProgressBarFillType::BottomToTop);
	SweetSpotBar->SetPercent(1.f);

	// Sweet spot is positioned from the bottom — canvas Y grows downward, so top offset = (1 - max) * BarHeight.
	float const SweetTop = (1.f - SweetSpotMaxRatio) * BarHeight;
	float const SweetHeight = (SweetSpotMaxRatio - SweetSpotMinRatio) * BarHeight;

	UCanvasPanelSlot* SweetSlot = Canvas->AddChildToCanvas(SweetSpotBar);
	SweetSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 0.f));
	SweetSlot->SetOffsets(FMargin(0.f, SweetTop, 0.f, SweetHeight));
	SweetSlot->SetAlignment(FVector2D(0.f, 0.f));
	SweetSlot->SetAutoSize(false);

	UGeoWidgetBuilderUtil::FinishBuild(WidgetBlueprint);

	UE_LOG(LogTemp, Log, TEXT("GeoHudWidgetBuilderUtil: Built WBP_ChargeBeamGauge (sweet spot %.2f–%.2f)"),
		   SweetSpotMinRatio, SweetSpotMaxRatio);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoHudWidgetBuilderUtil::AddAbilityBarToOverlay(UWidgetBlueprint* WidgetBlueprint, FName ParentPanelName,
													  TSubclassOf<UUserWidget> AbilityBarClass, float WidthFraction,
													  float HeightFraction, float BottomFraction)
{
	UCanvasPanelSlot* BarSlot = UGeoWidgetBuilderUtil::AddChildToCanvasPanel(WidgetBlueprint, ParentPanelName,
																			 AbilityBarClass, TEXT("AbilityBar"));
	if (!BarSlot)
	{
		return;
	}

	// Resolution-independent: anchors are canvas fractions, so the bar always covers the same screen proportion. The
	// anchor rectangle is WidthFraction wide, centered horizontally, and HeightFraction tall sitting BottomFraction above
	// the bottom edge. Zero offsets make the bar exactly fill that anchor rectangle at any resolution.
	float const HalfWidth = WidthFraction * 0.5f;
	float const Bottom = 1.f - BottomFraction;
	float const Top = Bottom - HeightFraction;
	BarSlot->SetAnchors(FAnchors(0.5f - HalfWidth, Top, 0.5f + HalfWidth, Bottom));
	BarSlot->SetAlignment(FVector2D(0.f, 0.f));
	BarSlot->SetAutoSize(false);
	BarSlot->SetOffsets(FMargin(0.f, 0.f, 0.f, 0.f));

	UGeoWidgetBuilderUtil::FinishBuild(WidgetBlueprint);

	UE_LOG(LogTemp, Log, TEXT("GeoHudWidgetBuilderUtil: Added AbilityBar (%s) to '%s' bottom-center (%.0f%%x%.0f%% of screen)"),
		   *AbilityBarClass->GetName(), *WidgetBlueprint->GetName(), WidthFraction * 100.f, HeightFraction * 100.f);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoHudWidgetBuilderUtil::BuildCombattantLifeBarWidget(UWidgetBlueprint* WidgetBlueprint, float BarWidth,
															float BarHeight)
{
	// Preserve the existing bar dimensions if the asset already has a sized root, so the rebuild keeps the same footprint.
	if (UWidgetTree* OldTree = WidgetBlueprint->WidgetTree)
	{
		OldTree->ForEachWidget(
			[&BarWidth, &BarHeight](UWidget* Widget)
			{
				if (USizeBox* SizeBox = Cast<USizeBox>(Widget))
				{
					if (SizeBox->IsWidthOverride())
					{
						BarWidth = SizeBox->GetWidthOverride();
					}
					if (SizeBox->IsHeightOverride())
					{
						BarHeight = SizeBox->GetHeightOverride();
					}
				}
			});
	}

	UWidgetTree* Tree = UGeoWidgetBuilderUtil::BeginBuild(WidgetBlueprint, TEXT("BuildCombattantLifeBarWidget"));
	if (!Tree)
	{
		return;
	}

	// Root SizeBox fixes the bar footprint; the Overlay stacks the shield bar over the health bar in the same rect.
	USizeBox* Root =
		Cast<USizeBox>(UGeoWidgetBuilderUtil::ConstructRootPanel(Tree, USizeBox::StaticClass(), TEXT("Root")));
	Root->SetWidthOverride(BarWidth);
	Root->SetHeightOverride(BarHeight);

	UOverlay* Stack = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Stack"));
	Root->AddChild(Stack);

	// HealthBar — fills the rect; its fill color is set at runtime by UGenericCombattantWidget::UpdateHealthRatio (white
	// default, opaque background so the empty portion reads as the depleted track).
	UProgressBar* HealthBar = UGeoWidgetBuilderUtil::ConstructProgressBar(
		Tree, TEXT("HealthBar"), FLinearColor::White, FLinearColor(1.f, 1.f, 1.f, 1.f), /*bIsVariable*/ true);
	UGeoWidgetBuilderUtil::AddFillChildToOverlay(Stack, HealthBar);

	// ShieldBar — same rect over the health bar, semi-transparent cyan fill so the health shows through, transparent
	// background so it draws only the filled portion. Percent is shield-as-fraction-of-max-health (UpdateShieldRatio).
	UProgressBar* ShieldBar =
		UGeoWidgetBuilderUtil::ConstructProgressBar(Tree, TEXT("ShieldBar"), FLinearColor(0.4f, 0.8f, 1.f, 0.6f),
													FLinearColor(0.f, 0.f, 0.f, 0.f), /*bIsVariable*/ true);
	UGeoWidgetBuilderUtil::AddFillChildToOverlay(Stack, ShieldBar);

	UGeoWidgetBuilderUtil::FinishBuild(WidgetBlueprint);

	UE_LOG(LogTemp, Log, TEXT("GeoHudWidgetBuilderUtil: Built WBP_CombattantLifeBar (%.0fx%.0f, shield over health)"),
		   BarWidth, BarHeight);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoHudWidgetBuilderUtil::BuildLocalConnectWidget(UWidgetBlueprint* WidgetBlueprint,
													   TSubclassOf<UUserWidget> MenuButtonClass)
{
	if (!ensureMsgf(MenuButtonClass && MenuButtonClass->IsChildOf(UGeoMenuButton::StaticClass()),
					TEXT("BuildLocalConnectWidget — MenuButtonClass must derive from UGeoMenuButton")))
	{
		return;
	}

	UWidgetTree* Tree = UGeoWidgetBuilderUtil::BeginBuild(WidgetBlueprint, TEXT("BuildLocalConnectWidget"));
	if (!Tree)
	{
		return;
	}

	// Root Overlay fills the panel rect; the VerticalBox is centered inside it so the stack of controls sits in the middle.
	UOverlay* Root = Cast<UOverlay>(UGeoWidgetBuilderUtil::ConstructRootPanel(Tree, UOverlay::StaticClass(), TEXT("Root")));

	UVerticalBox* Menu = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Menu"));
	if (UOverlaySlot* MenuSlot = Cast<UOverlaySlot>(Root->AddChildToOverlay(Menu)))
	{
		MenuSlot->SetHorizontalAlignment(HAlign_Center);
		MenuSlot->SetVerticalAlignment(VAlign_Center);
	}

	FMargin const RowPadding(0.f, 8.f);

	auto AddMenuButton = [Tree, Menu, &MenuButtonClass, &RowPadding](FName Name, TCHAR const* Label)
	{
		UGeoMenuButton* Button = Cast<UGeoMenuButton>(Tree->ConstructWidget<UUserWidget>(MenuButtonClass, Name));
		Button->Label = FText::FromString(Label);
		Button->bIsVariable = true;
		UGeoWidgetBuilderUtil::AddCenteredChildToVerticalBox(Menu, Button, RowPadding);
	};

	AddMenuButton(TEXT("HostButton"), TEXT("Host"));

	UEditableTextBox* IPInput = Tree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("IPInput"));
	IPInput->bIsVariable = true;
	// Defaults to loopback so Join works out of the box for same-machine testing.
	IPInput->SetText(FText::FromString(TEXT("127.0.0.1")));
	IPInput->SetHintText(FText::FromString(TEXT("Host IP")));
	IPInput->SetJustification(ETextJustify::Center);
	// The default theme draws light-gray text on the light box — force black, sized to match the menu buttons.
	FEditableTextBoxStyle Style = IPInput->GetWidgetStyle();
	Style.SetForegroundColor(FSlateColor(FLinearColor::Black));
	Style.TextStyle.Font.Size = 22;
	IPInput->SetWidgetStyle(Style);
	UGeoWidgetBuilderUtil::AddCenteredChildToVerticalBox(Menu, IPInput, RowPadding);

	AddMenuButton(TEXT("JoinButton"), TEXT("Join"));

	UTextBlock* LocalIPText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LocalIPText"));
	LocalIPText->bIsVariable = true;
	UGeoWidgetBuilderUtil::AddCenteredChildToVerticalBox(Menu, LocalIPText, RowPadding);

	AddMenuButton(TEXT("BackButton"), TEXT("Back"));

	UGeoWidgetBuilderUtil::FinishBuild(WidgetBlueprint);

	UE_LOG(LogTemp, Log, TEXT("GeoHudWidgetBuilderUtil: Built WBP_LocalConnect (centered Play Local panel)"));
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoHudWidgetBuilderUtil::AddLocalConnectToMainMenu(UWidgetBlueprint* WidgetBlueprint, FName ParentPanelName,
														 FName ButtonsBoxName, TSubclassOf<UUserWidget> MenuButtonClass,
														 TSubclassOf<UUserWidget> LocalConnectClass)
{
	if (!ensureMsgf(WidgetBlueprint && LocalConnectClass, TEXT("AddLocalConnectToMainMenu — null arg")) ||
		!ensureMsgf(MenuButtonClass && MenuButtonClass->IsChildOf(UGeoMenuButton::StaticClass()),
					TEXT("AddLocalConnectToMainMenu — MenuButtonClass must derive from UGeoMenuButton")))
	{
		return;
	}

	UWidgetTree* Tree = WidgetBlueprint->WidgetTree;
	UVerticalBox* ButtonsBox = Tree ? Cast<UVerticalBox>(Tree->FindWidget(ButtonsBoxName)) : nullptr;
	if (!ensureMsgf(ButtonsBox, TEXT("AddLocalConnectToMainMenu — no VerticalBox named '%s' in '%s'"),
					*ButtonsBoxName.ToString(), *WidgetBlueprint->GetName()))
	{
		return;
	}

	Tree->Modify();
	WidgetBlueprint->Modify();

	// Re-run-safe: drop a previous run's widgets so the add rebuilds cleanly instead of duplicating.
	if (UWidget* Existing = Tree->FindWidget(TEXT("PlayLocalButton")))
	{
		Existing->RemoveFromParent();
	}
	if (UWidget* Existing = Tree->FindWidget(TEXT("PlayLocalSpacer")))
	{
		Existing->RemoveFromParent();
	}

	// Insert PlayLocalButton + spacer just above QuitButton, keeping the box's button/spacer rhythm.
	UGeoMenuButton* PlayLocalButton =
		Cast<UGeoMenuButton>(Tree->ConstructWidget<UUserWidget>(MenuButtonClass, TEXT("PlayLocalButton")));
	PlayLocalButton->Label = FText::FromString(TEXT("Play Local"));
	PlayLocalButton->bIsVariable = true;

	USpacer* Spacer = Tree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("PlayLocalSpacer"));
	Spacer->SetSize(FVector2D(1.f, 10.f));

	int32 InsertIndex = ButtonsBox->GetChildIndex(Tree->FindWidget(TEXT("QuitButton")));
	if (InsertIndex == INDEX_NONE)
	{
		InsertIndex = ButtonsBox->GetChildrenCount();
	}
	ButtonsBox->InsertChildAt(InsertIndex, Spacer);
	ButtonsBox->InsertChildAt(InsertIndex, PlayLocalButton);

	// Appending to a built asset: the compiler only mints GUIDs when the map is empty, and its verify pass requires one
	// for EVERY added widget (variable or not), so register both here (AddChildToCanvasPanel does it for
	// LocalConnectWidget below).
	WidgetBlueprint->WidgetVariableNameToGuidMap.Add(TEXT("PlayLocalButton"), FGuid::NewGuid());
	WidgetBlueprint->WidgetVariableNameToGuidMap.Add(TEXT("PlayLocalSpacer"), FGuid::NewGuid());

	UCanvasPanelSlot* PanelSlot = UGeoWidgetBuilderUtil::AddChildToCanvasPanel(WidgetBlueprint, ParentPanelName,
																			   LocalConnectClass, TEXT("LocalConnectWidget"));
	if (!PanelSlot)
	{
		return;
	}
	PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
	PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	PanelSlot->SetAutoSize(true);

	UGeoWidgetBuilderUtil::FinishBuild(WidgetBlueprint);

	UE_LOG(LogTemp, Log, TEXT("GeoHudWidgetBuilderUtil: Added PlayLocalButton + LocalConnectWidget (%s) to '%s'"),
		   *LocalConnectClass->GetName(), *WidgetBlueprint->GetName());
}
