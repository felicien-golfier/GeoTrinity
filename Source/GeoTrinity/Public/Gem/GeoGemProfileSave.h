// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Characters/PlayerClassTypes.h"
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Gem/GeoGemTypes.h"

#include "GeoGemProfileSave.generated.h"

class UGeoGemCatalog;

/** One saved page of a class's sockets: a whole loadout under the name the player gave it. */
USTRUCT()
struct FGeoGemBuild
{
	GENERATED_BODY()

	/** Empty until the player names it; the menus then show a default name from its place in the list. */
	UPROPERTY()
	FString Name;

	UPROPERTY()
	FGeoGemLoadout Loadout;
};

/** One class's progress: its level, which opens sockets, the XP earned towards the next one, and its builds. */
USTRUCT()
struct FGeoGemClassProgress
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Level = 1;

	/** XP earned since reaching Level; always 0 at the max level. */
	UPROPERTY()
	int32 Xp = 0;

	/** Never empty once the profile is loaded: the class fights with Builds[ActiveBuild]. */
	UPROPERTY()
	TArray<FGeoGemBuild> Builds;

	UPROPERTY()
	int32 ActiveBuild = 0;

	/** The one loadout of a profile saved before builds existed; moved into the first build on load. */
	UPROPERTY()
	FGeoGemLoadout Loadout;
};

/**
 * One local player's gems, shards and class progress, kept on their own machine in AppData (see
 * GeoLib::LoadUserSaveFile). Also holds every rule that changes them, so equipping and the Forge check the same
 * counts. The gem stacks are shared by the three classes, and the loadouts share copies too: one copy can sit in a
 * socket of every build of every class at once, so a class's free count is owned - slotted in the build it plays.
 * Breaking down only takes copies no build needs: owned - the most any one build slots. Each class keeps up to
 * GeoGem::MaxBuilds builds; the one it plays is the one the menus edit and the server applies.
 */
UCLASS()
class GEOTRINITY_API UGeoGemProfileSave : public USaveGame
{
	GENERATED_BODY()

public:
	/** Loads LocalPlayerIndex's profile, or a fresh one the first time. Each couch-coop player keeps their own file. */
	static UGeoGemProfileSave* Load(int32 LocalPlayerIndex);

	/** Writes this profile back to LocalPlayerIndex's file. */
	void Save(int32 LocalPlayerIndex);

	/** Total shards available to spend in the Forge. */
	int32 GetShards() const { return Shards; }
	/** Total copies of GemId this player owns, across loadouts and unslotted inventory. */
	int32 GetOwnedCount(FName GemId) const { return OwnedGems.FindRef(GemId); }

	/** Copies of GemId PlayerClass can still equip: owned - those the build it plays already slots or echoes. */
	int32 GetFreeCount(FName GemId, EPlayerClass PlayerClass) const;

	/** Copies of GemId the Forge may break down: owned - the most any one build slots or echoes. */
	int32 GetBreakableCount(FName GemId) const;

	/** Current level of PlayerClass, from 1 up to GeoGem::MaxClassLevel. */
	int32 GetClassLevel(EPlayerClass PlayerClass) const;

	/** XP PlayerClass earned since its current level. */
	int32 GetClassXp(EPlayerClass PlayerClass) const;

	/** The gem in the echo socket of the build PlayerClass plays; NAME_None when empty. */
	FName GetEcho(EPlayerClass PlayerClass) const;

	/** The echo level shared by every class: 0 until a class first reaches GeoGem::EchoUnlockLevel, then up to
	 * Catalog.EchoMaxLevel. */
	int32 GetEchoLevel() const;

	/** XP earned since the current echo level; keeps growing at the max echo level. */
	int32 GetEchoXp() const;

	/**
	 * How far PlayerClass is towards its next level, 0 .. 1, the same bar below and at the max class level: XP into the
	 * class level, then XP into the echo level. OutXpTaken / OutXpNeeded are that bar's numbers; a class at the max
	 * echo level reads full.
	 */
	float GetXpProgress(UGeoGemCatalog const& Catalog, EPlayerClass PlayerClass, int32& OutXpTaken,
						int32& OutXpNeeded) const;

	/**
	 * Whether SetEcho would take GemId into the echo socket of the build PlayerClass plays: not when the echo level is
	 * still 0, GemId is a Core or already the echo, or no copy is owned.
	 */
	bool CanEcho(UGeoGemCatalog const& Catalog, EPlayerClass PlayerClass, FName GemId) const;

	/** Echoes a copy of GemId, emptying the sockets of the build that hold it. Refused (false) when CanEcho is not. */
	bool SetEcho(UGeoGemCatalog const& Catalog, EPlayerClass PlayerClass, FName GemId);

	/** Empties the echo socket of the build PlayerClass plays; the copy goes back to free. */
	void ClearEcho(EPlayerClass PlayerClass);

	/** The loadout of the build PlayerClass plays, one entry per GeoGem::GetSockets() index. */
	FGeoGemLoadout GetLoadout(EPlayerClass PlayerClass) const;

	/** PlayerClass's builds, at least one. */
	TArray<FGeoGemBuild> GetBuilds(EPlayerClass PlayerClass) const;

	/** Index in GetBuilds of the build PlayerClass plays. */
	int32 GetActiveBuild(EPlayerClass PlayerClass) const;

	/** Makes PlayerClass play its build BuildIndex. */
	void SetActiveBuild(EPlayerClass PlayerClass, int32 BuildIndex);

	/** Appends an empty, unnamed build to PlayerClass and plays it. Refused (false) at GeoGem::MaxBuilds. */
	bool AddBuild(EPlayerClass PlayerClass);

	/** Deletes the build PlayerClass plays, which then plays the one before it. Refused on its last build. */
	void RemoveActiveBuild(EPlayerClass PlayerClass);

	/** Names the build PlayerClass plays, cut to GeoGem::MaxBuildNameLength; empty gives back its default name. */
	void RenameActiveBuild(EPlayerClass PlayerClass, FString const& Name);

	/** Adds Count copies of GemId to its stack: loot, or a cheat. */
	void AddGems(FName GemId, int32 Count);

	/**
	 * Sets PlayerClass's level, clamped to [1, GeoGem::MaxClassLevel], with no XP towards the next. Lowering it empties
	 * every socket of every build that is locked again at the new level; the copies go back to free. A class at the max
	 * level lifts the echo level to 1; once no class is at the max level the echo level and XP go back to 0 and every
	 * echo socket empties.
	 */
	void SetClassLevel(EPlayerClass PlayerClass, int32 Level);

#if !UE_BUILD_SHIPPING
	/** Dev: sets the shared echo level, clamped 0 .. Catalog.EchoMaxLevel; the echo XP restarts at 0. A level above 0
	 * with no class at the max level first takes Triangle there. */
	void SetEchoLevel(UGeoGemCatalog const& Catalog, int32 Level);
#endif

	/**
	 * Adds Xp to PlayerClass, leveling it up every Catalog.XpPerLevel up to the max level; what is earned at the max
	 * level goes to the shared echo XP, raising the echo level (each costing Catalog.GetEchoXpForLevel) up to
	 * Catalog.EchoMaxLevel, where the XP is still kept. Returns the class levels gained.
	 */
	int32 AddClassXp(UGeoGemCatalog const& Catalog, EPlayerClass PlayerClass, int32 Xp);

	/**
	 * Whether Equip would take GemId into socket SocketIndex of the build PlayerClass plays: not when the socket is
	 * still locked at the class level, is of another tier or already holds GemId, no copy is free, GemId is the build's
	 * echo, or GemId is a Core this build already holds.
	 */
	bool CanEquip(UGeoGemCatalog const& Catalog, EPlayerClass PlayerClass, int32 SocketIndex, FName GemId) const;

	/** Slots a free copy of GemId into PlayerClass's socket SocketIndex, returning whatever it held to free. Refused
	 * (false) when CanEquip is not. */
	bool Equip(UGeoGemCatalog const& Catalog, EPlayerClass PlayerClass, int32 SocketIndex, FName GemId);

	/** Empties PlayerClass's socket SocketIndex; the copy goes back to free. */
	void Unequip(EPlayerClass PlayerClass, int32 SocketIndex);

	/**
	 * Equips GemId into every socket of PlayerClass it may go in, empty or not (a slotted gem is replaced), while free
	 * copies last. Returns how many.
	 */
	int32 FillSockets(UGeoGemCatalog const& Catalog, EPlayerClass PlayerClass, FName GemId);

	/** Breaks up to Quantity breakable copies of GemId down into shards. Returns the shards gained. */
	int32 BreakDown(UGeoGemCatalog const& Catalog, FName GemId, int32 Quantity);

	/** Breaks every breakable copy of every Tier gem down into shards. Returns the shards gained. */
	int32 BreakDownTier(UGeoGemCatalog const& Catalog, EGeoGemTier Tier);

	/** Spends shards on Quantity copies of GemId. Refused (false) when the shards do not cover it. */
	bool Craft(UGeoGemCatalog const& Catalog, FName GemId, int32 Quantity);

private:
	/** GeoGems.sav for the first local player, GeoGems_P<n>.sav for the n-th couch-coop one. */
	static FString GetFileName(int32 LocalPlayerIndex);

	/** PlayerClass's progress, created at level 1 the first time, with at least one build, each sized to every
	 * socket. */
	FGeoGemClassProgress& FindOrAddClass(EPlayerClass PlayerClass);

	/** The loadout of the build PlayerClass plays, its sockets sized to every socket. */
	FGeoGemLoadout& GetActiveLoadout(EPlayerClass PlayerClass);

	/** Whether any class is at GeoGem::EchoUnlockLevel. */
	bool HasClassAtEchoUnlockLevel() const;

	/** The echo follows the class levels: any class at GeoGem::EchoUnlockLevel lifts the echo level from 0 to 1; none
	 * resets the echo level and XP to 0 and empties every build's echo socket. */
	void SyncEchoWithClassLevels();

	/** Copies of GemId Loadout holds: its sockets plus the echo. */
	static int32 CountSlotted(FGeoGemLoadout const& Loadout, FName GemId);

	UPROPERTY()
	TMap<FName, int32> OwnedGems;

	UPROPERTY()
	int32 Shards = 0;

	UPROPERTY()
	TMap<EPlayerClass, FGeoGemClassProgress> Classes;

	/** One echo level for the whole profile, 0 until a class reaches the max level; kept across builds and classes. */
	UPROPERTY()
	int32 EchoLevel = 0;

	/** Every XP point earned at the max class level since reaching EchoLevel; never reset at the max echo level. */
	UPROPERTY()
	int32 EchoXp = 0;
};
