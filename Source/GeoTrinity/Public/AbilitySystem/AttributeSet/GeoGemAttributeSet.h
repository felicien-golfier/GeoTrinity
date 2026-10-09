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

	UPROPERTY(BlueprintReadOnly, Category = "GeoGem", ReplicatedUsing = OnRep_DashDistanceMultiplier)
	FGameplayAttributeData DashDistanceMultiplier;
	ATTRIBUTE_ACCESSORS(UGeoGemAttributeSet, DashDistanceMultiplier)

	/** Scales how long the player's deployables blink before they end. */
	UPROPERTY(BlueprintReadOnly, Category = "GeoGem", ReplicatedUsing = OnRep_DeployableBlinkMultiplier)
	FGameplayAttributeData DeployableBlinkMultiplier;
	ATTRIBUTE_ACCESSORS(UGeoGemAttributeSet, DeployableBlinkMultiplier)

	UPROPERTY(BlueprintReadOnly, Category = "GeoGem", ReplicatedUsing = OnRep_DeployableHealthMultiplier)
	FGameplayAttributeData DeployableHealthMultiplier;
	ATTRIBUTE_ACCESSORS(UGeoGemAttributeSet, DeployableHealthMultiplier)

	/** Scales how fast the player's deployables drain their own health. */
	UPROPERTY(BlueprintReadOnly, Category = "GeoGem", ReplicatedUsing = OnRep_DeployableDrainMultiplier)
	FGameplayAttributeData DeployableDrainMultiplier;
	ATTRIBUTE_ACCESSORS(UGeoGemAttributeSet, DeployableDrainMultiplier)

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
};
