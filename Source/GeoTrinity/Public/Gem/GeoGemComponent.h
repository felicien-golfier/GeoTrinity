// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "ActiveGameplayEffectHandle.h"
#include "Abilities/GameplayAbilityTypes.h"
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

	/** Owning client: sends every class's loadout in Profile to the server. */
	void SendLoadoutsToServer(UGeoGemProfileSave const& Profile);

	/** Server: replaces the gem stats and the gem passive abilities on the owner's ASC with those of the current class's
	 *  loadout. Called by APlayableCharacter::ResetAttributes, which refills Health and Ammo after it. */
	void ApplyGems();

	/** Server: hands Reward to the owning client, whose profile keeps the gems and the class XP. */
	void GrantReward(FGeoGemReward const& Reward);

private:
	/** Stores PlayerClass's loadout, applied right away when it is the current class's and no fight is running; one
	 *  sent mid-fight waits for the next GiveLife. */
	UFUNCTION(Server, Reliable)
	void ServerSetLoadout(EPlayerClass PlayerClass, FGeoGemLoadout const& Loadout);

	/** Adds Reward to the owning player's profile and saves it. */
	UFUNCTION(Client, Reliable)
	void ClientGrantReward(FGeoGemReward const& Reward);

	UPROPERTY()
	TMap<EPlayerClass, FGeoGemLoadout> Loadouts;

	FActiveGameplayEffectHandle StatsEffectHandle;

	/** The passive abilities ApplyGems gave, to take back before it gives the loadout's. */
	TArray<FGameplayAbilitySpecHandle> GrantedAbilityHandles;
};
