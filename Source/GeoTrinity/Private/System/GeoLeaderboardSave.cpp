// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "System/GeoLeaderboardSave.h"

#include "Tool/UGeoGameplayLibrary.h"

// ---------------------------------------------------------------------------------------------------------------------
bool FGeoAttemptEntry::operator<(FGeoAttemptEntry const& Other) const
{
	if (BossHealthRatio != Other.BossHealthRatio)
	{
		return BossHealthRatio < Other.BossHealthRatio;
	}
	return DurationSeconds < Other.DurationSeconds;
}

// ---------------------------------------------------------------------------------------------------------------------
UGeoLeaderboardSave* UGeoLeaderboardSave::Load()
{
	return CastChecked<UGeoLeaderboardSave>(GeoLib::LoadUserSaveFile(FileName, StaticClass()));
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoLeaderboardSave::Record(FGeoAttemptEntry const& Entry)
{
	UGeoLeaderboardSave* Leaderboard = Load();
	// Every machine in the fight records the attempt; under PIE they are all this one, writing the one file.
	bool const bAlreadyRecorded = Leaderboard->Entries.ContainsByPredicate(
		[&Entry](FGeoAttemptEntry const& Recorded)
		{
			return Recorded.AttemptId == Entry.AttemptId;
		});
	if (bAlreadyRecorded)
	{
		return;
	}

	Leaderboard->Entries.Add(Entry);
	Leaderboard->Entries.Sort();

	GeoLib::WriteUserSaveFile(Leaderboard, FileName);
}
