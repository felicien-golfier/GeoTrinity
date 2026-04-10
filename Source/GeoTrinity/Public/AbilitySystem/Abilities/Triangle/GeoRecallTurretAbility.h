// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/GeoGameplayAbility.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Tool/Team.h"

#include "GeoRecallTurretAbility.generated.h"


class AGeoTurret;
class UEffectDataAsset;

/**
 * Recalls all deployed turrets. Each recall instantly applies effects to the player
 * and fires a GameplayCue for the visual (beam from turret to player).
 * Uses base EffectDataAssets/EffectDataInstances for the recall effect.
 * Turrets that were blinking additionally apply BlinkBonusEffectData (expected to hold a
 * FContextDamageMultiplierEffectData).
 */
UCLASS()
class GEOTRINITY_API UGeoRecallTurretAbility : public UGeoGameplayAbility
{
	GENERATED_BODY()

	struct FRecallInfo
	{
		TWeakObjectPtr<AGeoTurret> Turret;
		FVector TurretLocation;
		bool bWasBlinking;
	};

protected:
	/**
	 * Returns the list of player ASCs that should receive the recall effect.
	 * Uses OverlapAttitude to filter by team (hostile ASCs = players who deal or receive the buff).
	 */
	TArray<UGeoAbilitySystemComponent*> FindTargets(AActor const* Instigator, FRecallInfo const& RecallInfo) const;
	/** Recalls all deployed turrets, applies effects per turret, and fires the recall gameplay cue. */
	virtual void Fire(FGeoAbilityTargetData const& AbilityTargetData) override;
	/**
	 * Executes the recall GameplayCue on PlayerASC with the turret location as the cue origin.
	 *
	 * @param PlayerASC       ASC of the target player receiving the visual cue.
	 * @param RecallInfo      Info about the turret being recalled (location, blink state).
	 * @param AvatarLocation  Location of the player character (beam end point).
	 */
	void FireRecallCue(UGeoAbilitySystemComponent* PlayerASC, FRecallInfo const& RecallInfo,
					   FVector const& AvatarLocation) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Recall")
	TArray<TInstancedStruct<FEffectData>> BlinkBonusEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Recall", meta = (Categories = "GameplayCue"))
	FGameplayTag RecallGameplayCueTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly,
			  meta = (Bitmask, BitmaskEnum = "/Script/GeoTrinity.ETeamAttitudeBitflag"))
	int32 OverlapAttitude = static_cast<int32>(ETeamAttitudeBitflag::Hostile);
};
