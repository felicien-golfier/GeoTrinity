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

	/** Copies of GemId PlayerClass can still equip: owned - those the build it plays already slots. */
	int32 GetFreeCount(FName GemId, EPlayerClass PlayerClass) const;

	/** Copies of GemId the Forge may break down: owned - the most any one build slots. */
	int32 GetBreakableCount(FName GemId) const;

	/** Current level of PlayerClass, from 1 up to GeoGem::MaxClassLevel. */
	int32 GetClassLevel(EPlayerClass PlayerClass) const;

	/** XP PlayerClass earned since its current level. */
	int32 GetClassXp(EPlayerClass PlayerClass) const;

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

	/** Sets PlayerClass's level, clamped to [1, GeoGem::MaxClassLevel], with no XP towards the next. */
	void SetClassLevel(EPlayerClass PlayerClass, int32 Level);

	/** Adds Xp to PlayerClass, leveling it up every Catalog.XpPerLevel up to the max level. Returns the levels gained. */
	int32 AddClassXp(UGeoGemCatalog const& Catalog, EPlayerClass PlayerClass, int32 Xp);

	/**
	 * Whether Equip would take GemId into socket SocketIndex of the build PlayerClass plays: not when the socket is
	 * still locked at the class level, is of another tier or already holds GemId, no copy is free, or GemId is a Core
	 * this build already holds.
	 */
	bool CanEquip(UGeoGemCatalog const& Catalog, EPlayerClass PlayerClass, int32 SocketIndex, FName GemId) const;

	/** Slots a free copy of GemId into PlayerClass's socket SocketIndex, returning whatever it held to free. Refused
	 * (false) when CanEquip is not. */
	bool Equip(UGeoGemCatalog const& Catalog, EPlayerClass PlayerClass, int32 SocketIndex, FName GemId);

	/** Empties PlayerClass's socket SocketIndex; the copy goes back to free. */
	void Unequip(EPlayerClass PlayerClass, int32 SocketIndex);

	/** Equips GemId into every empty socket of PlayerClass it may go in, while free copies last. Returns how many. */
	int32 FillEmptySockets(UGeoGemCatalog const& Catalog, EPlayerClass PlayerClass, FName GemId);

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

	/** The sockets of the build PlayerClass plays, sized to every socket. */
	TArray<FName>& GetActiveSockets(EPlayerClass PlayerClass);

	UPROPERTY()
	TMap<FName, int32> OwnedGems;

	UPROPERTY()
	int32 Shards = 0;

	UPROPERTY()
	TMap<EPlayerClass, FGeoGemClassProgress> Classes;
};
