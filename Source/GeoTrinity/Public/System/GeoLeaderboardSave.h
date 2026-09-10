// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Characters/PlayerClassTypes.h"
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GameplayTagContainer.h"
#include "Misc/Guid.h"

#include "GeoLeaderboardSave.generated.h"

/** One player as they were when a fight ended: the name their machine is logged in under (the Steam nickname, when
 *  Steam is the online subsystem) and the class they played. */
USTRUCT()
struct FGeoLeaderboardPlayer
{
	GENERATED_BODY()

	UPROPERTY()
	FString PlayerName;

	UPROPERTY()
	EPlayerClass PlayerClass = EPlayerClass::None;
};

/** One finished attempt at a boss: how long it lasted, how much of the boss was left, and who was in it. */
USTRUCT()
struct FGeoLeaderboardEntry
{
	GENERATED_BODY()

	/** Identifies the attempt across the machines that each record it, so a file more than one of them
	 *  writes — PIE runs every instance against this computer's — keeps one entry rather than one per machine. */
	UPROPERTY()
	FGuid AttemptId;

	/** Arena.* tag of the encounter, so attempts at different bosses stay tellable apart in one list. */
	UPROPERTY()
	FGameplayTag ArenaTag;

	/** Seconds from the fight starting to it ending — the value the boss bar's timer stopped on. */
	UPROPERTY()
	float DurationSeconds = 0.f;

	/** Boss health left when the fight ended, as a fraction: 0 is a kill, 1 a boss never touched. */
	UPROPERTY()
	float BossHealthRatio = 1.f;

	UPROPERTY()
	TArray<FGeoLeaderboardPlayer> Players;

	/** Ranks attempts: least boss health left first (a kill leads), ties broken by the faster attempt. */
	bool operator<(FGeoLeaderboardEntry const& Other) const;
};

/**
 * The local leaderboard, kept in the user's AppData rather than in a save slot: a slot lands under the project's
 * Saved folder, which every packaged build brings its own fresh copy of, so the runs would be gone with each new
 * build. The file holds standard USaveGame bytes, only written and read by hand.
 * Every machine that took part in a fight records the same attempt into its own copy — the server builds it and
 * multicasts it, so a player who joined someone else's server still keeps their run (see AGeoArena::RecordAttempt).
 * PIE is the one place those machines share a file, so an attempt is recorded once there rather than once per
 * instance.
 */
UCLASS()
class GEOTRINITY_API UGeoLeaderboardSave : public USaveGame
{
	GENERATED_BODY()

public:
	/** Loads the leaderboard from disk, or hands back an empty one the first time. Never null. */
	static UGeoLeaderboardSave* Load();

	/** Adds Entry in ranking order and writes the leaderboard back to disk, unless it is already recorded. */
	static void Record(FGeoLeaderboardEntry const& Entry);

	/** Every attempt recorded on this machine, best first. */
	UPROPERTY()
	TArray<FGeoLeaderboardEntry> Entries;

private:
	/** <AppData>/Local/<Project>/GeoLeaderboard.sav — the one place the file's location is written down. */
	static FString FilePath();
};
