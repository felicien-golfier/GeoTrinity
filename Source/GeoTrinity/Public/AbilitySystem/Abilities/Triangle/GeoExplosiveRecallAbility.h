// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/GeoGameplayAbility.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "GeoExplosiveRecallAbility.generated.h"

class UEffectDataAsset;

/**
 * Recalls all deployed turrets. Each recall instantly applies effects to the player
 * and fires a GameplayCue for the visual (beam from turret to player).
 * Turrets that were blinking (low duration) use BlinkBonusRecallEffectData.
 */
UCLASS()
class GEOTRINITY_API UGeoExplosiveRecallAbility : public UGeoGameplayAbility
{
	GENERATED_BODY()

protected:
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
								 FGameplayAbilityActivationInfo ActivationInfo,
								 FGameplayEventData const* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Recall")
	TObjectPtr<UEffectDataAsset> NormalRecallEffectData;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Recall")
	TObjectPtr<UEffectDataAsset> BlinkBonusRecallEffectData;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Recall", meta = (Categories = "GameplayCue"))
	FGameplayTag RecallGameplayCueTag;
};
