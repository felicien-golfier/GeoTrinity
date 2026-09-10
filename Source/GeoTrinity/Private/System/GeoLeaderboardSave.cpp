// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "System/GeoLeaderboardSave.h"

#include "GeoTrinity/GeoTrinity.h"
#include "HAL/PlatformProcess.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

// ---------------------------------------------------------------------------------------------------------------------
bool FGeoLeaderboardEntry::operator<(FGeoLeaderboardEntry const& Other) const
{
	if (BossHealthRatio != Other.BossHealthRatio)
	{
		return BossHealthRatio < Other.BossHealthRatio;
	}
	return DurationSeconds < Other.DurationSeconds;
}

// ---------------------------------------------------------------------------------------------------------------------
FString UGeoLeaderboardSave::FilePath()
{
	return FPaths::Combine(FPlatformProcess::UserSettingsDir(), FApp::GetProjectName(), TEXT("GeoLeaderboard.sav"));
}

// ---------------------------------------------------------------------------------------------------------------------
UGeoLeaderboardSave* UGeoLeaderboardSave::Load()
{
	TArray<uint8> SaveData;
	// No file yet is the normal first run, not a failure.
	if (FFileHelper::LoadFileToArray(SaveData, *FilePath()))
	{
		if (UGeoLeaderboardSave* Saved = Cast<UGeoLeaderboardSave>(UGameplayStatics::LoadGameFromMemory(SaveData)))
		{
			return Saved;
		}
		UE_LOG(LogGeoTrinity, Warning, TEXT("%hs: %s could not be read — starting a fresh leaderboard"), __FUNCTION__,
			   *FilePath());
	}
	return CastChecked<UGeoLeaderboardSave>(
		UGameplayStatics::CreateSaveGameObject(UGeoLeaderboardSave::StaticClass()));
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoLeaderboardSave::Record(FGeoLeaderboardEntry const& Entry)
{
	UGeoLeaderboardSave* Leaderboard = Load();
	// Every machine in the fight records the attempt; under PIE they are all this one, writing the one file.
	bool const bAlreadyRecorded = Leaderboard->Entries.ContainsByPredicate(
		[&Entry](FGeoLeaderboardEntry const& Recorded)
		{
			return Recorded.AttemptId == Entry.AttemptId;
		});
	if (bAlreadyRecorded)
	{
		return;
	}

	Leaderboard->Entries.Add(Entry);
	Leaderboard->Entries.Sort();

	TArray<uint8> SaveData;
	bool const bWritten = UGameplayStatics::SaveGameToMemory(Leaderboard, SaveData) &&
						  FFileHelper::SaveArrayToFile(SaveData, *FilePath());
	ensureMsgf(bWritten, TEXT("%hs: failed to write %s"), __FUNCTION__, *FilePath());
}
