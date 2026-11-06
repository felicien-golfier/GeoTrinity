// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Characters/PlayerClassTypes.h"
#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"

#include "GeoGemSubsystem.generated.h"

class AGeoPlayerState;
class UGeoGemProfileSave;
struct FGeoGemReward;

DECLARE_MULTICAST_DELEGATE(FGeoGemProfileCommittedSignature);

/**
 * Owns one local player's gem profile for the whole session. Whatever edits the profile (the Gems menu, loot, a
 * cheat) calls one of SaveChanges / CommitChanges / CommitAll afterwards, which is what reaches the disk and the server.
 */
UCLASS()
class GEOTRINITY_API UGeoGemSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	/** Loads this local player's profile. */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** This local player's gem profile for the session; never null after Initialize. */
	UGeoGemProfileSave* GetProfile() const { return Profile; }

	/** The gem profile of World's first local player; null without one (a dedicated server). */
	static UGeoGemProfileSave* GetFirstLocalProfile(UWorld const* World);

	/** Saves the profile and broadcasts OnProfileCommitted; sends nothing. For edits the server does not read (Forge
	 * craft / break: owned counts and shards). */
	void SaveChanges();

	/** SaveChanges, plus PlayerClass's loadout sent to the server, when this player is connected to one. For edits to
	 * that class's build. */
	void CommitChanges(EPlayerClass PlayerClass);

	/** SaveChanges, plus the echo level and every class's loadout sent to the server. For rewards, class / echo level
	 * changes and dev commands. */
	void CommitAll();

	/** Fires after every SaveChanges / CommitChanges / CommitAll: the profile (levels, XP, gems) may have changed. */
	FGeoGemProfileCommittedSignature OnProfileCommitted;

	/** Adds a finished attempt's class XP and gems to the profile, then commits it. */
	void GrantReward(FGeoGemReward const& Reward);

private:
	/** The local player's player state; null while not connected to a match. */
	AGeoPlayerState const* GetLocalPlayerState() const;

	UPROPERTY()
	TObjectPtr<UGeoGemProfileSave> Profile;
};
