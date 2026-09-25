// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Tool/GeoColor.h"

#include "Engine/Texture2D.h"
#include "Settings/GameDataSettings.h"

// ---------------------------------------------------------------------------------------------------------------------
FLinearColor FGeoColorParam::GetColor(float const Alpha) const
{
	FLinearColor ReturnColor = FLinearColor::White;
	if (Color == EGeoColor::Override)
	{
		ReturnColor = OverrideColor;
	}
	else
	{
		FLinearColor const* const PaletteColor = GetDefault<UGameDataSettings>()->ColorPalette.Find(Color);
		if (ensureMsgf(PaletteColor, TEXT("FGeoColorParam: %s has no entry in the Game Data Settings color palette"),
					   *UEnum::GetValueAsString(Color)))
		{
			ReturnColor = *PaletteColor;
		}
	}

	if (Alpha >= 0)
	{
		ReturnColor.A = Alpha;
	}

	return ReturnColor;
}

// ---------------------------------------------------------------------------------------------------------------------
TArray<FLinearColor> GeoColor::GetMeaningColors(FGeoColorParam const& Color,
												TArray<FGeoColorParam> const& SecondaryColors)
{
	ensureMsgf(SecondaryColors.Num() < MaxMeaningColorCount,
			   TEXT("GeoColor: %d secondary colours, an effect shows at most %d colours"), SecondaryColors.Num(),
			   MaxMeaningColorCount);

	TArray<FLinearColor> Colors = {Color.GetColor()};
	for (int32 Index = 0; Index < SecondaryColors.Num() && Colors.Num() < MaxMeaningColorCount; ++Index)
	{
		Colors.Add(SecondaryColors[Index].GetColor());
	}
	return Colors;
}

// ---------------------------------------------------------------------------------------------------------------------
UTexture2D* GeoColor::CreatePaletteTexture()
{
	TArray<FFloat16Color, TInlineAllocator<SlotCount>> Texels;
	Texels.Reserve(SlotCount);
	for (int32 Slot = 0; Slot < SlotCount; ++Slot)
	{
		FGeoColorParam SlotParam;
		SlotParam.Color = static_cast<EGeoColor>(Slot);
		Texels.Add(FFloat16Color(SlotParam.GetColor()));
	}

	UTexture2D* const PaletteTexture =
		UTexture2D::CreateTransient(SlotCount, 1, PF_FloatRGBA, TEXT("GeoColorPalette"),
									TConstArrayView64<uint8>(reinterpret_cast<uint8 const*>(Texels.GetData()),
															 static_cast<int64>(Texels.Num()) * sizeof(FFloat16Color)));

	// Point sampling and clamping: neighbouring texels are unrelated colors, so any blend between them is meaningless.
	PaletteTexture->Filter = TF_Nearest;
	PaletteTexture->AddressX = TA_Clamp;
	PaletteTexture->AddressY = TA_Clamp;
	PaletteTexture->SRGB = false;
	PaletteTexture->UpdateResource();
	return PaletteTexture;
}
