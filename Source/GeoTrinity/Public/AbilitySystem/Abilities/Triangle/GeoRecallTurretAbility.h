// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
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
	 * Returns the ASCs of interactable agents lying on the turret-to-player line (LineHalfWidth wide).
	 * Uses OverlapAttitude to filter by team (hostile ASCs = players who deal or receive the buff).
	 */
	TArray<UGeoAbilitySystemComponent*> FindTargets(AActor const* Instigator, FRecallInfo const& RecallInfo) const;
	/** Recalls all deployed turrets, applies effects per turret, and fires the recall gameplay cue. */
	virtual void Fire(FGeoAbilityTargetData const& AbilityTargetData) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Recall")
	TArray<TInstancedStruct<FEffectData>> BlinkBonusEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly,
			  meta = (Bitmask, BitmaskEnum = "/Script/GeoTrinity.ETeamAttitudeBitflag"))
	int32 OverlapAttitude = TeamAttitudeMask::HostileOrNeutral;

	/** Half-width of the recall line, added to each target's collision radius when testing hits. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Recall")
	float LineHalfWidth = 50.f;
};
