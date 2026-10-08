// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Detail/GeoIconCustomization.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "HUD/Style/SGeoIconImage.h"
#include "Tool/GeoIcon.h"
#include "Widgets/Layout/SBox.h"

// ---------------------------------------------------------------------------------------------------------------------
TSharedRef<IDetailCustomization> FGeoIconCustomization::MakeInstance()
{
	return MakeShared<FGeoIconCustomization>();
}

// ---------------------------------------------------------------------------------------------------------------------
void FGeoIconCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailBuilder.GetObjectsBeingCustomized(Objects);
	UGeoIcon const* Icon = Objects.Num() == 1 ? Cast<UGeoIcon>(Objects[0].Get()) : nullptr;
	if (!Icon)
	{
		return;
	}

	TSharedRef<SGeoIconImage> IconImage = SNew(SGeoIconImage);
	IconImage->SetIcon(Icon, PreviewSize, FLinearColor::White);

	DetailBuilder.EditCategory("GeoIconPreview", INVTEXT("Preview"), ECategoryPriority::Important)
		.AddCustomRow(INVTEXT("Preview"))
		.WholeRowContent()
		.HAlign(HAlign_Left)
		[
			SNew(SBox)
			.Padding(FMargin(8.f))
			[
				IconImage
			]
		];
}
