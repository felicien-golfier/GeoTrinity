// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GeoAttributeSetBase.h"

#include "GeoGemAttributeSet.generated.h"

/**
 * Stats only gems change, on AGeoPlayerState's ASC beside UCharacterAttributeSet. UGeoGemStatsEffect lists every
 * attribute of this set, so a new gem stat is one attribute here plus its catalog row. Each starts at its no-gem
 * value: 1 for a multiplier, so a gem's Add of 0.03 makes it 1.03.
 */
UCLASS()
class GEOTRINITY_API UGeoGemAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	/** Sets every attribute to its no-gem value. */
	UGeoGemAttributeSet();

	/** Registers all gem stat attributes for replication. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Chance of a hit or a heal to crit, once the Critical Core unlocks crits. */
	UPROPERTY(BlueprintReadOnly, Category = "GeoGem", ReplicatedUsing = OnRep_CritChance)
	FGameplayAttributeData CritChance;
	ATTRIBUTE_ACCESSORS(UGeoGemAttributeSet, CritChance)

	/** What a crit multiplies its damage or heal by. */
	UPROPERTY(BlueprintReadOnly, Category = "GeoGem", ReplicatedUsing = OnRep_CritDamage)
	FGameplayAttributeData CritDamage;
	ATTRIBUTE_ACCESSORS(UGeoGemAttributeSet, CritDamage)

	/** Scales how far the player's dash travels. */
	UPROPERTY(BlueprintReadOnly, Category = "GeoGem", ReplicatedUsing = OnRep_DashDistanceMultiplier)
	FGameplayAttributeData DashDistanceMultiplier;
	ATTRIBUTE_ACCESSORS(UGeoGemAttributeSet, DashDistanceMultiplier)

	/** Scales how long the player's deployables blink before they end. */
	UPROPERTY(BlueprintReadOnly, Category = "GeoGem", ReplicatedUsing = OnRep_DeployableBlinkMultiplier)
	FGameplayAttributeData DeployableBlinkMultiplier;
	ATTRIBUTE_ACCESSORS(UGeoGemAttributeSet, DeployableBlinkMultiplier)

	/** Scales the health of the player's deployables on deploy. */
	UPROPERTY(BlueprintReadOnly, Category = "GeoGem", ReplicatedUsing = OnRep_DeployableHealthMultiplier)
	FGameplayAttributeData DeployableHealthMultiplier;
	ATTRIBUTE_ACCESSORS(UGeoGemAttributeSet, DeployableHealthMultiplier)

	/** Scales how fast the player's deployables drain their own health. */
	UPROPERTY(BlueprintReadOnly, Category = "GeoGem", ReplicatedUsing = OnRep_DeployableDrainMultiplier)
	FGameplayAttributeData DeployableDrainMultiplier;
	ATTRIBUTE_ACCESSORS(UGeoGemAttributeSet, DeployableDrainMultiplier)

	/** Scales the cooldown of the Ability.Type.Dash ability. */
	UPROPERTY(BlueprintReadOnly, Category = "GeoGem", ReplicatedUsing = OnRep_DashCooldownMultiplier)
	FGameplayAttributeData DashCooldownMultiplier;
	ATTRIBUTE_ACCESSORS(UGeoGemAttributeSet, DashCooldownMultiplier)

	/** Scales the cooldown of every Ability.Type.Special ability. */
	UPROPERTY(BlueprintReadOnly, Category = "GeoGem", ReplicatedUsing = OnRep_SpecialCooldownMultiplier)
	FGameplayAttributeData SpecialCooldownMultiplier;
	ATTRIBUTE_ACCESSORS(UGeoGemAttributeSet, SpecialCooldownMultiplier)

	/** Scales how long an Ability.Type.Deployable ability takes to recharge one charge. */
	UPROPERTY(BlueprintReadOnly, Category = "GeoGem", ReplicatedUsing = OnRep_DeployableCooldownMultiplier)
	FGameplayAttributeData DeployableCooldownMultiplier;
	ATTRIBUTE_ACCESSORS(UGeoGemAttributeSet, DeployableCooldownMultiplier)

	/** Scales the FireDelay of every ability but the reload: its wind-up, a charge's full charge time, an automatic
	 * fire's shot interval. */
	UPROPERTY(BlueprintReadOnly, Category = "GeoGem", ReplicatedUsing = OnRep_WindUpMultiplier)
	FGameplayAttributeData WindUpMultiplier;
	ATTRIBUTE_ACCESSORS(UGeoGemAttributeSet, WindUpMultiplier)

	/** Divides the FireDelay of the Ability.Type.Reload ability, its reload time. */
	UPROPERTY(BlueprintReadOnly, Category = "GeoGem", ReplicatedUsing = OnRep_ReloadSpeedMultiplier)
	FGameplayAttributeData ReloadSpeedMultiplier;
	ATTRIBUTE_ACCESSORS(UGeoGemAttributeSet, ReloadSpeedMultiplier)

	/** Scales how far the player's spells reach: projectile travel, beam and ray length, deploy throw. */
	UPROPERTY(BlueprintReadOnly, Category = "GeoGem", ReplicatedUsing = OnRep_SpellDistanceMultiplier)
	FGameplayAttributeData SpellDistanceMultiplier;
	ATTRIBUTE_ACCESSORS(UGeoGemAttributeSet, SpellDistanceMultiplier)

protected:
	UFUNCTION()
	void OnRep_CritChance(FGameplayAttributeData const& OldCritChance);
	UFUNCTION()
	void OnRep_CritDamage(FGameplayAttributeData const& OldCritDamage);
	UFUNCTION()
	void OnRep_DashDistanceMultiplier(FGameplayAttributeData const& OldDashDistanceMultiplier);
	UFUNCTION()
	void OnRep_DeployableBlinkMultiplier(FGameplayAttributeData const& OldDeployableBlinkMultiplier);
	UFUNCTION()
	void OnRep_DeployableHealthMultiplier(FGameplayAttributeData const& OldDeployableHealthMultiplier);
	UFUNCTION()
	void OnRep_DeployableDrainMultiplier(FGameplayAttributeData const& OldDeployableDrainMultiplier);
	UFUNCTION()
	void OnRep_DashCooldownMultiplier(FGameplayAttributeData const& OldDashCooldownMultiplier);
	UFUNCTION()
	void OnRep_SpecialCooldownMultiplier(FGameplayAttributeData const& OldSpecialCooldownMultiplier);
	UFUNCTION()
	void OnRep_DeployableCooldownMultiplier(FGameplayAttributeData const& OldDeployableCooldownMultiplier);
	UFUNCTION()
	void OnRep_WindUpMultiplier(FGameplayAttributeData const& OldWindUpMultiplier);
	UFUNCTION()
	void OnRep_ReloadSpeedMultiplier(FGameplayAttributeData const& OldReloadSpeedMultiplier);
	UFUNCTION()
	void OnRep_SpellDistanceMultiplier(FGameplayAttributeData const& OldSpellDistanceMultiplier);
};
