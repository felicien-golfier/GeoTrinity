// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
#include "CoreMinimal.h"

#include "GeoCorePassiveAbility.generated.h"

/**
 * Base of the passive ability a Core gem gives its holder (FGeoGemInfo::GrantedAbility). Active from its grant, it binds
 * its rule to the delegate of the event the rule reacts to (BindEvent) and unbinds it when it ends, so the code raising
 * the event never names a Core. Each Core's Blueprint child carries the Ability.Type.Passive and Ability.Spell.<Core>
 * asset tags, the AbilityInfo entry and the rule's tunables.
 */
UCLASS(Abstract)
class GEOTRINITY_API UGeoCorePassiveAbility : public UGeoGameplayAbility
{
	GENERATED_BODY()

public:
	/** One server-owned instance per player, which keeps the cooldown: a client cancel request must never end the
	 *  server's instance after a revive. */
	UGeoCorePassiveAbility();

protected:
	/** Binds the rule through BindEvent. */
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
								 FGameplayAbilityActivationInfo ActivationInfo,
								 FGameplayEventData const* TriggerEventData) override;
	/** Unbinds the rule through UnbindEvent before calling Super. */
	virtual void EndAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
							FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
							bool bWasCancelled) override;

	/** Binds the rule to the delegate of its event, on the machines the rule runs on. HolderASC is the holder's. */
	virtual void BindEvent(UGeoAbilitySystemComponent& HolderASC) PURE_VIRTUAL(UGeoCorePassiveAbility::BindEvent, );
	/** Undoes BindEvent. */
	virtual void UnbindEvent(UGeoAbilitySystemComponent& HolderASC) PURE_VIRTUAL(UGeoCorePassiveAbility::UnbindEvent, );

	/** Server, for a rule that fires on an event (Wake, Split, Kinship): true, and starts the cooldown, when the last true
	 *  was at least CooldownSeconds ago and the TriggerChance roll passes. */
	bool TryTrigger();

	/** Seconds after the rule fires before it can fire again; 0 for a rule with no cooldown. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|Core", meta = (ClampMin = "0"))
	float CooldownSeconds = 0.f;

	/** Chance the rule fires each time its event happens off cooldown; 1 always fires. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|Core", meta = (ClampMin = "0", ClampMax = "1"))
	float TriggerChance = 1.f;

private:
	float LastTriggerTime = TNumericLimits<float>::Lowest();
};
