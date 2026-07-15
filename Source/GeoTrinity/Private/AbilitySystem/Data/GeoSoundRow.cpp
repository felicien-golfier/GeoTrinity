// Copyright 2024 GeoTrinity. All Rights Reserved.


#include "AbilitySystem/Data/GeoSoundRow.h"

FGeoSoundRow UGeoSoundRowLibrary::FindSoundForTag(UDataTable const* SoundTable, FGameplayTag Tag, bool& bFound)
{
	bFound = false;
	FGeoSoundRow FoundRow;
	if (!ensureMsgf(SoundTable, TEXT("FindSoundForTag called with null SoundTable")))
	{
		return FoundRow;
	}

	SoundTable->ForeachRow<FGeoSoundRow>(TEXT("FindSoundForTag"),
		[&Tag, &FoundRow, &bFound](FName const& /*RowName*/, FGeoSoundRow const& Row)
		{
			if (!bFound && Row.Tag.MatchesTagExact(Tag))
			{
				bFound = true;
				FoundRow = Row;
			}
		});
	return FoundRow;
}
