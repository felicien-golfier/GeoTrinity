// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Style/GeoIconImage.h"

#include "HUD/Style/SGeoIconImage.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoIconImage::SetIcon(UGeoIcon const* InIcon)
{
	Icon = InIcon;
	SynchronizeProperties();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoIconImage::SetSize(float const InSize)
{
	Size = InSize;
	SynchronizeProperties();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoIconImage::SetTint(FLinearColor const& InTint)
{
	Tint = InTint;
	SynchronizeProperties();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoIconImage::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (MyIcon)
	{
		MyIcon->SetIcon(Icon, Size, Tint);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoIconImage::ReleaseSlateResources(bool const bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyIcon.Reset();
}

#if WITH_EDITOR
// ---------------------------------------------------------------------------------------------------------------------
FText const UGeoIconImage::GetPaletteCategory()
{
	return INVTEXT("Geo");
}
#endif

// ---------------------------------------------------------------------------------------------------------------------
TSharedRef<SWidget> UGeoIconImage::RebuildWidget()
{
	MyIcon = SNew(SGeoIconImage);
	return MyIcon.ToSharedRef();
}
