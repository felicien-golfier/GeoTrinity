// Copyright 2024 GeoTrinity. All Rights Reserved.


#include "AbilitySystem/Data/GeoGenericSoundCueNotify.h"

#include "AbilitySystem/Data/GeoSoundRow.h"

bool UGeoGenericSoundCueNotify::OnExecute_Implementation(AActor* Target, FGameplayCueParameters const& Parameters) const
{
	if (!ensureMsgf(SoundTable, TEXT("%s: SoundTable is not set"), *GetName()))
	{
		return Super::OnExecute_Implementation(Target, Parameters);
	}

	for (FGameplayTag const& SoundTag : Parameters.AggregatedSourceTags)
	{
		bool bFound = false;
		FGeoSoundRow const Row = UGeoSoundRowLibrary::FindSoundForTag(SoundTable, SoundTag, bFound);
		if (bFound)
		{
			UGeoSoundRowLibrary::PlaySoundEntry2D(Target, Row.Entry, Parameters.GetInstigator());
		}
	}

	return Super::OnExecute_Implementation(Target, Parameters);
}
