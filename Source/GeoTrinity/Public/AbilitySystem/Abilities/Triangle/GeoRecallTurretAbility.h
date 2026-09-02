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

	/** Blocks activation while the avatar owns no still-active turret (plus the base death check). */
	virtual bool CanActivateAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
									FGameplayTagContainer const* SourceTags = nullptr,
									FGameplayTagContainer const* TargetTags = nullptr,
									FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/** Returns Avatar's turrets a recall would actually pull back: still tracked, still active (an expired one lingers
	 * in the manager until its delayed destroy). Tracking is fed by BeginPlay, so clients agree with the server. */
	TArray<AGeoTurret*> GetActiveTurrets(AActor const* Avatar) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoAbility|Recall")
	TArray<TInstancedStruct<FEffectData>> BlinkBonusEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly,
			  meta = (Bitmask, BitmaskEnum = "/Script/GeoTrinity.ETeamAttitudeBitflag"))
	int32 OverlapAttitude = TeamAttitudeMask::HostileOrNeutral;

	/** Half-width of the recall line, added to each target's collision radius when testing hits. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoAbility|Recall")
	float LineHalfWidth = 50.f;
};
