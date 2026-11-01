// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "System/Stats/GeoLeaderboardSave.h"

#include "Algo/StableSort.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Settings/GameDataSettings.h"
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
	UGeoLeaderboardSave* Leaderboard =
		CastChecked<UGeoLeaderboardSave>(GeoLib::LoadUserSaveFile(FileName, StaticClass()));
	Leaderboard->Prune();

	FString const BuildTimeStamp = IFileManager::Get().GetTimeStamp(FPlatformProcess::ExecutablePath()).ToString();
	if (Leaderboard->ApplyBuildWipe(GetDefault<UGameDataSettings>()->bWipeLeaderboardOnNewBuild, BuildTimeStamp))
	{
		GeoLib::WriteUserSaveFile(Leaderboard, FileName);
	}
	return Leaderboard;
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
	if (!bAlreadyRecorded)
	{
		Leaderboard->AddAttempt(Entry);
		GeoLib::WriteUserSaveFile(Leaderboard, FileName);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoLeaderboardSave::AddAttempt(FGeoAttemptEntry const& Entry)
{
	Entries.Add(Entry);
	Prune();
	return Entries.ContainsByPredicate(
		[&Entry](FGeoAttemptEntry const& Recorded)
		{
			return Recorded.AttemptId == Entry.AttemptId;
		});
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoLeaderboardSave::Prune()
{
	// Stable, so of two equal attempts the earlier recorded stays first and wins the tie.
	Algo::StableSort(Entries);

	TArray<FGeoAttemptEntry> Kept;
	for (FGeoAttemptEntry const& Entry : Entries)
	{
		bool const bBestForSomeone = Entry.Players.ContainsByPredicate(
			[&Kept, &Entry](FGeoLeaderboardPlayer const& Player)
			{
				return !Kept.ContainsByPredicate(
					[&Entry, &Player](FGeoAttemptEntry const& Better)
					{
						return Better.ArenaTag == Entry.ArenaTag && Better.Difficulty == Entry.Difficulty
							&& Better.Players.ContainsByPredicate(
								[&Player](FGeoLeaderboardPlayer const& BetterPlayer)
								{
									return BetterPlayer.PlayerName == Player.PlayerName;
								});
					});
			});
		if (bBestForSomeone)
		{
			Kept.Add(Entry);
		}
	}
	Entries = MoveTemp(Kept);
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoLeaderboardSave::ApplyBuildWipe(bool const bWipe, FString const& CurrentStamp)
{
	bool const bWipes = bWipe && CurrentStamp != BuildStamp;
	if (bWipes)
	{
		Entries.Empty();
		BuildStamp = CurrentStamp;
	}
	return bWipes;
}
