// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Style/GeoUITheme.h"

#include "Components/TextBlock.h"
#include "HUD/Style/GeoUISettings.h"
#include "Settings/GameDataSettings.h"

// ---------------------------------------------------------------------------------------------------------------------
UGeoUITheme const* UGeoUITheme::Get()
{
	TSoftObjectPtr<UGeoUITheme> const& Theme = GetDefault<UGeoUISettings>()->Theme;
	if (!ensureMsgf(!Theme.IsNull(), TEXT("%hs: no theme set in Project Settings > Geo UI"), __FUNCTION__))
	{
		return nullptr;
	}

	return UGameDataSettings::GetLoadedDataAsset(Theme);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoUITheme::ApplyTextStyle(UTextBlock* Text, EGeoTextRole const Role)
{
	UGeoUITheme const* Theme = Role != EGeoTextRole::Custom ? Get() : nullptr;
	FGeoTextStyle const* Style = Theme ? Theme->FindTextStyle(Role) : nullptr;
	if (Style)
	{
		Text->SetFont(Style->Font);
		Text->SetColorAndOpacity(Style->Color);
		Text->SetTextTransformPolicy(Style->Transform);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
FGeoTextStyle const* UGeoUITheme::FindTextStyle(EGeoTextRole const Role) const
{
	FGeoTextStyle const* Style = TextStyles.Find(Role);
	ensureMsgf(Style, TEXT("%hs: %s defines no %s text style"), __FUNCTION__, *GetName(),
			   *UEnum::GetValueAsString(Role));
	return Style;
}

// ---------------------------------------------------------------------------------------------------------------------
FGeoClassStyle const* UGeoUITheme::FindClassStyle(EPlayerClass const PlayerClass) const
{
	FGeoClassStyle const* Style = ClassStyles.Find(PlayerClass);
	ensureMsgf(Style, TEXT("%hs: %s defines no %s class style"), __FUNCTION__, *GetName(),
			   *UEnum::GetValueAsString(PlayerClass));
	return Style;
}
