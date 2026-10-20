// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Tutorial/GeoCatFaceWidget.h"

#include "HUD/Tutorial/SGeoCatFace.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCatFaceWidget::SetTalking(bool const bInTalking)
{
	bTalking = bInTalking;
	SynchronizeProperties();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCatFaceWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (MyFace)
	{
		MyFace->SetBase(Base);
		MyFace->SetShade(Shade);
		MyFace->SetInk(Ink);
		MyFace->SetLight(Light);
		MyFace->SetSize(Size);
		MyFace->SetTalking(bTalking);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCatFaceWidget::ReleaseSlateResources(bool const bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyFace.Reset();
}

#if WITH_EDITOR
// ---------------------------------------------------------------------------------------------------------------------
FText const UGeoCatFaceWidget::GetPaletteCategory()
{
	return INVTEXT("Geo");
}
#endif

// ---------------------------------------------------------------------------------------------------------------------
TSharedRef<SWidget> UGeoCatFaceWidget::RebuildWidget()
{
	MyFace = SNew(SGeoCatFace);
	return MyFace.ToSharedRef();
}
