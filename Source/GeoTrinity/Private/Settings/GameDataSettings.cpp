// Copyright 2024 GeoTrinity. All Rights Reserved.


#include "Settings/GameDataSettings.h"

UClass* UGameDataSettings::FindCombattantWidgetComponentClass() const
{
	return FindObject<UClass>(nullptr, *CombattantWidgetComponentClass.ToString());
}
