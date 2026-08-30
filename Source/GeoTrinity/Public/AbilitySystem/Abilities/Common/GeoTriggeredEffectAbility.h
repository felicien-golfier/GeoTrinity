// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
#include "CoreMinimal.h"

#include "GeoTriggeredEffectAbility.generated.h"

/** Who has to have landed the hit for it to trigger, every hit reported to the passive being the owner's already. */
UENUM()
enum class EGeoHitTriggerSource : uint8
{
	/** Only what the character fired itself. */
	Instigator,
	/** Only what the actors it deployed fired: its turret, its mines. */
	OwnedActor,
	/** Either of the two, and nothing else. */
	Any
};

/**
 * Applies its own effect data to its owner whenever one of the watched abilities fires: a hit landed by an ability in
 * HitTriggerTags (through the ASC's OnAbilityHit, or every ability but those when bInvertHitTriggerTags), or the
 * activation of one in ActivationTriggerTags.
 *
 * Which abilities trigger it and what they grant are both data, so one class covers every "on hit / on cast, gain X".
 */
UCLASS()
class GEOTRINITY_API UGeoTriggeredEffectAbility : public UGeoGameplayAbility
{
	GENERATED_BODY()

public:
	/** Sets NetSecurityPolicy to ServerOnly — a client cancel request must never end the server's passive instance. */
	UGeoTriggeredEffectAbility();

private:
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
								 FGameplayAbilityActivationInfo ActivationInfo,
								 FGameplayEventData const* TriggerEventData) override;
	virtual void EndAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
							FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
							bool bWasCancelled) override;

	/** Applies GetEffectDataArray() from the owner onto itself. */
	void ApplyEffectsToSelf();

	UFUNCTION()
	void OnAbilityHitCallback(FGameplayTag AbilityTag, AActor* HitInstigator, AActor* HitActor);
	void OnAbilityActivatedCallback(UGameplayAbility* Ability);

	/** Abilities whose hits trigger the effects. A parent tag here matches every ability below it. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|Effects", meta = (Categories = "Ability.Spell"))
	FGameplayTagContainer HitTriggerTags;

	/** Turns HitTriggerTags into a blocklist: every hit the owner lands triggers the effects except those abilities. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|Effects")
	bool bInvertHitTriggerTags = false;

	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|Effects")
	EGeoHitTriggerSource HitTriggerSource = EGeoHitTriggerSource::Instigator;

	/** Abilities whose activation triggers the effects, whether or not they go on to hit anything. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|Effects", meta = (Categories = "Ability.Spell"))
	FGameplayTagContainer ActivationTriggerTags;
};
