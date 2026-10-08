// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Thumbnail/GeoIconThumbnailRenderer.h"

#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "GlobalRenderResources.h"
#include "HUD/Style/SGeoIconImage.h"
#include "Input/HittestGrid.h"
#include "RenderDeferredCleanup.h"
#include "Slate/WidgetRenderer.h"
#include "Tool/GeoIcon.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SVirtualWindow.h"

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoIconThumbnailRenderer::CanVisualizeAsset(UObject* Object)
{
	return Object && Object->IsA<UGeoIcon>();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoIconThumbnailRenderer::Draw(UObject* Object, int32 const X, int32 const Y, uint32 const Width,
									 uint32 const Height, FRenderTarget* /*RenderTarget*/, FCanvas* Canvas,
									 bool /*bAdditionalViewFamily*/)
{
	UGeoIcon const* Icon = Cast<UGeoIcon>(Object);
	if (!Icon || Width < 1 || Height < 1 || !FApp::CanEverRender())
	{
		return;
	}

	FCanvasTileItem Backdrop(FVector2D(X, Y), GWhiteTexture, FVector2D(Width, Height), FLinearColor(.02f, .02f, .03f));
	Backdrop.Draw(Canvas);
	Canvas->Flush_GameThread();

	FVector2D const Size(Width, Height);
	TSharedRef<SGeoIconImage> IconImage = SNew(SGeoIconImage);
	IconImage->SetIcon(Icon, Size.GetMin(), FLinearColor::White);
	TSharedRef<SVirtualWindow> Window = SNew(SVirtualWindow);
	Window->SetContent(SNew(SBox).Padding(.12f * Size.GetMin())[IconImage]);
	Window->Resize(Size);

	FHittestGrid HitTestGrid;
	FWidgetRenderer* WidgetRenderer = new FWidgetRenderer(true);
	WidgetRenderer->SetShouldClearTarget(false);
	WidgetRenderer->ViewOffset = FVector2D(X, Y);
	WidgetRenderer->DrawWindow(Canvas->GetRenderTarget(), HitTestGrid, Window, 1.f, Size, 0.f);
	BeginCleanup(WidgetRenderer);
}
