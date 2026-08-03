// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "World/GeoWorldSettings.h"

#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialParameterCollection.h"
#include "Settings/GameDataSettings.h"
#include "Tool/GeoColor.h"
#include "Tool/UGeoGameplayLibrary.h"

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoWorldSettings::BeginPlay()
{
	Super::BeginPlay();

	if (GeoLib::IsDedicatedServer(this))
	{
		return;
	}

	UGameDataSettings const* const Settings = GetDefault<UGameDataSettings>();
	UMaterialParameterCollection* const PaletteCollection = Settings->ColorPaletteCollection.LoadSynchronous();
	if (!ensureMsgf(PaletteCollection,
					TEXT("AGeoWorldSettings: no ColorPaletteCollection in Game Data Settings — every material reading "
						 "a palette slot renders black")))
	{
		return;
	}

	UEnum const* const ColorEnum = StaticEnum<EGeoColor>();
	for (TPair<EGeoColor, FLinearColor> const& Slot : Settings->ColorPalette)
	{
		UKismetMaterialLibrary::SetVectorParameterValue(
			this, PaletteCollection, FName(*ColorEnum->GetNameStringByValue(static_cast<int64>(Slot.Key))), Slot.Value);
	}
}
