// Copyright 2024 GeoTrinity. All Rights Reserved.

#if WITH_EDITOR

#include "Tool/GeoWidgetBuilderUtil.h"

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

	return Tree;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoWidgetBuilderUtil::FinishBuild(UWidgetBlueprint* WidgetBlueprint)
{
	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
	UEditorLoadingAndSavingUtils::SavePackages({WidgetBlueprint->GetPackage()}, false);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoWidgetBuilderUtil::BuildChargeBeamGaugeWidget(UWidgetBlueprint* WidgetBlueprint, float SweetSpotMinRatio,
													   float SweetSpotMaxRatio)
{
	UWidgetTree* Tree = BeginBuild(WidgetBlueprint, TEXT("BuildChargeBeamGaugeWidget"));
	if (!Tree)
	{
		return;
	}

	// --- Root: Canvas Panel ---
	UCanvasPanel* Canvas = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasRoot"));
	Tree->RootWidget = Canvas;

	constexpr float BarHeight = 80.f;

	// --- ChargeBar: full-width, dark-blue fill ---
	UProgressBar* ChargeBar = Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("ChargeBar"));
	{
		FProgressBarStyle Style = ChargeBar->GetWidgetStyle();
		Style.BackgroundImage.TintColor = FSlateColor(FLinearColor(0.1f, 0.4f, 1.0f, .5f));
		Style.FillImage.TintColor = FSlateColor(FLinearColor(0.1f, 0.4f, 1.0f, 1.0f));
		ChargeBar->SetWidgetStyle(Style);
	}
	ChargeBar->SetFillColorAndOpacity(FLinearColor::White);
	ChargeBar->SetBarFillType(EProgressBarFillType::BottomToTop);
	ChargeBar->SetPercent(0.f);

	UCanvasPanelSlot* ChargeSlot = Canvas->AddChildToCanvas(ChargeBar);
	ChargeSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	ChargeSlot->SetOffsets(FMargin(0.f, 0.f, 0.f, 0.f));
	ChargeSlot->SetAlignment(FVector2D(0.f, 0.f));

	// --- SweetSpotBar: narrow golden overlay at the sweet-spot window ---
	UProgressBar* SweetSpotBar = Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("SweetSpotBar"));
	{
		FProgressBarStyle Style = SweetSpotBar->GetWidgetStyle();
		Style.BackgroundImage.TintColor = FSlateColor(FLinearColor(1.f, 0.75f, 0.0f, 0.5f));
		Style.FillImage.TintColor = FSlateColor(FLinearColor(1.0f, 0.75f, 0.0f, 1.0f));
		SweetSpotBar->SetWidgetStyle(Style);
	}
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

	FinishBuild(WidgetBlueprint);

	UE_LOG(LogTemp, Log, TEXT("GeoWidgetBuilderUtil: Built WBP_ChargeBeamGauge (sweet spot %.2f–%.2f)"),
		   SweetSpotMinRatio, SweetSpotMaxRatio);
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
