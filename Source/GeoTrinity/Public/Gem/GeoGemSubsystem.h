// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"

#include "GeoGemSubsystem.generated.h"

class UGeoGemProfileSave;
struct FGeoGemReward;

/**
 * Owns one local player's gem profile for the whole session. Whatever edits the profile (the Gems menu, loot, a
 * cheat) calls CommitChanges afterwards, which is what reaches the disk and the server.
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

	/** Saves the profile and sends its loadouts to the server, when this player is connected to one. */
	void CommitChanges();

	/** Adds a finished attempt's class XP and gems to the profile, then commits it. */
	void GrantReward(FGeoGemReward const& Reward);

private:
	UPROPERTY()
	TObjectPtr<UGeoGemProfileSave> Profile;
};
