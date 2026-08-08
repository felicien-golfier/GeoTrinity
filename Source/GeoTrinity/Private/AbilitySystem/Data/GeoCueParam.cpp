// Copyright 2024 GeoTrinity. All Rights Reserved.


#include "AbilitySystem/Data/GeoCueParam.h"

#include "GameplayEffectTypes.h"

void FGeoCueParam::FillCueParams(FGameplayCueParameters& CueParams) const
{
	CueParams.GameplayEffectLevel = static_cast<int32>(Color);
	if (SoundTag.IsValid())
	{
		CueParams.AggregatedSourceTags.AddTag(SoundTag);
	}
}
