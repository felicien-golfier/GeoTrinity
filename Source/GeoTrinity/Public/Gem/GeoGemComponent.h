// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Abilities/GameplayAbilityTypes.h"
#include "ActiveGameplayEffectHandle.h"
#include "Characters/PlayerClassTypes.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Gem/GeoGemTypes.h"

#include "GeoGemComponent.generated.h"

class UGeoGemProfileSave;

/**
 * Server half of the gems, on AGeoPlayerState. The gems themselves live in the owning client's UGeoGemProfileSave;
 * this keeps the loadouts that client last sent and applies the current class's one to the player's ASC. The server
 * takes the client's loadout on trust: the counts that decide what may be slotted are never sent.
 */
UCLASS(ClassGroup = "GeoTrinity")
class GEOTRINITY_API UGeoGemComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Replicated so the owning client can reach ServerSetLoadout. */
	UGeoGemComponent();

	/** Owning client: sends the echo level and every class's loadout in Profile to the server. */
	void SendAllToServer(UGeoGemProfileSave const& Profile);

	/** Owning client: sends PlayerClass's loadout in Profile to the server. */
	void SendLoadoutToServer(UGeoGemProfileSave const& Profile, EPlayerClass PlayerClass);

	/** Server: stores the shared echo level; the gem stats follow at the next SetLoadout or ApplyGems. */
	void SetEchoLevel(int32 NewEchoLevel) { EchoLevel = NewEchoLevel; }

	/** Server: stores PlayerClass's loadout. For the current class, with no fight running and a live pawn, re-applies
	 *  the gems and refills Health / Ammo (never the class's default attributes). */
	void SetLoadout(EPlayerClass PlayerClass, FGeoGemLoadout const& Loadout);

	/** Server: replaces the gem stats and the gem passive abilities on the owner's ASC with those of the current
	 * class's loadout. Called by APlayableCharacter::ResetAttributes and SetLoadout. */
	void ApplyGems();

	/** Server: the effects of the gems on the owner's ASC that death must not remove (the stats GE). Empty on a client.
	 *  A gem-granted effect appends its handle here. */
	TArray<FActiveGameplayEffectHandle> GetSurvivingEffectHandles() const;

	/** Server: the level the owning client last sent for PlayerClass; 1 until it has sent one. */
	int32 GetClassLevel(EPlayerClass const PlayerClass) const { return Loadouts.FindRef(PlayerClass).Level; }

	/** Server: the shared echo level the owning client last sent; 0 until it has sent one. */
	int32 GetEchoLevel() const { return EchoLevel; }

	/** Server: hands Reward to the owning client, whose profile keeps the gems and the class XP. */
	void GrantReward(FGeoGemReward const& Reward);

private:
	/** SetLoadout on the server; one sent mid-fight waits for the next GiveLife. */
	UFUNCTION(Server, Reliable)
	void ServerSetLoadout(EPlayerClass PlayerClass, FGeoGemLoadout const& Loadout);

	/** SetEchoLevel on the server. */
	UFUNCTION(Server, Reliable)
	void ServerSetEchoLevel(int32 NewEchoLevel);

	/** Adds Reward to the owning player's profile and saves it. */
	UFUNCTION(Client, Reliable)
	void ClientGrantReward(FGeoGemReward const& Reward);

	UPROPERTY()
	TMap<EPlayerClass, FGeoGemLoadout> Loadouts;

	int32 EchoLevel = 0;

	FActiveGameplayEffectHandle StatsEffectHandle;

	/** The passive abilities ApplyGems gave, to take back before it gives the loadout's. */
	TArray<FGameplayAbilitySpecHandle> GrantedAbilityHandles;
};
