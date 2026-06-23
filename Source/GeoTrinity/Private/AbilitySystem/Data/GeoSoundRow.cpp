// Copyright 2024 GeoTrinity. All Rights Reserved.


#include "AbilitySystem/Data/GeoSoundRow.h"

USoundBase* UGeoSoundRowLibrary::FindSoundForTag(UDataTable const* SoundTable, FGameplayTag Tag)
{
	if (!ensureMsgf(SoundTable, TEXT("FindSoundForTag called with null SoundTable")))
	{
		return nullptr;
	}

	USoundBase* FoundSound = nullptr;
	SoundTable->ForeachRow<FGeoSoundRow>(TEXT("FindSoundForTag"),
		[&Tag, &FoundSound](FName const& /*RowName*/, FGeoSoundRow const& Row)
		{
			if (!FoundSound && Row.Tag.MatchesTagExact(Tag))
			{
				FoundSound = Row.Sound;
			}
		});
	return FoundSound;
}
