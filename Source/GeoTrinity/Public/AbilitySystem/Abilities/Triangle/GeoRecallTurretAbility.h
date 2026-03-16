// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/GeoGameplayAbility.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Tool/UGameplayLibrary.h"

#include "GeoRecallTurretAbility.generated.h"

class UEffectDataAsset;

/**
 * Recalls all deployed turrets. Each recall instantly applies effects to the player
 * and fires a GameplayCue for the visual (beam from turret to player).
 * Turrets that were blinking (low duration) use BlinkBonusRecallEffectData.
 */
UCLASS()
class GEOTRINITY_API UGeoRecallTurretAbility : public UGeoGameplayAbility
{
	GENERATED_BODY()

	struct FRecallInfo
	{
		FVector TurretLocation;
		bool bWasBlinking;
	};

protected:
	TArray<UGeoAbilitySystemComponent*> FindTargets(AActor const* Instigator, FRecallInfo const& RecallInfo) const;
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
								 FGameplayAbilityActivationInfo ActivationInfo,
								 FGameplayEventData const* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Recall")
	TObjectPtr<UEffectDataAsset> NormalRecallEffectData;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Recall")
	TObjectPtr<UEffectDataAsset> BlinkBonusRecallEffectData;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Recall", meta = (Categories = "GameplayCue"))
	FGameplayTag RecallGameplayCueTag;

	UPROPERTY(EditAnywhere, meta = (Bitmask, BitmaskEnum = "/Script/GeoTrinity.ETeamAttitudeBitflag"))
	int32 OverlapAttitude = static_cast<int32>(ETeamAttitudeBitflag::Hostile);
};
