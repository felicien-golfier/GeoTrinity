// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoGemSocketButton.h"

#include "Components/ButtonSlot.h"
#include "HUD/Style/GeoGemGlyph.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemSocketButton::InitSocket(int32 const InSocketIndex, UGeoGemGlyph* InGlyph)
{
	SocketIndex = InSocketIndex;
	Glyph = InGlyph;

	FButtonStyle Style = GetStyle();
	Style.SetNormal(FSlateNoResource())
		.SetHovered(FSlateNoResource())
		.SetPressed(FSlateNoResource())
		.SetDisabled(FSlateNoResource())
		.SetNormalPadding(FMargin(0.f))
		.SetPressedPadding(FMargin(0.f));
	SetStyle(Style);

	Glyph->SetRenderTransformPivot(FVector2D(.5f, .5f));
	if (UButtonSlot* const GlyphSlot = Cast<UButtonSlot>(AddChild(Glyph)))
	{
		GlyphSlot->SetPadding(FMargin(0.f));
		GlyphSlot->SetHorizontalAlignment(HAlign_Fill);
		GlyphSlot->SetVerticalAlignment(VAlign_Fill);
	}

	OnClicked.AddUniqueDynamic(this, &UGeoGemSocketButton::HandleClicked);
	OnHovered.AddUniqueDynamic(this, &UGeoGemSocketButton::HandleHovered);
	OnUnhovered.AddUniqueDynamic(this, &UGeoGemSocketButton::HandleUnhovered);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemSocketButton::HandleClicked()
{
	OnPicked.ExecuteIfBound(SocketIndex);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemSocketButton::HandleHovered()
{
	Glyph->SetRenderScale(FVector2D(HoverScale));
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemSocketButton::HandleUnhovered()
{
	Glyph->SetRenderScale(FVector2D(1.f));
}
