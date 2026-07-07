// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/GeoChannelBeamAbility.h"
#include "AbilitySystem/Data/EffectData.h"
#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"

#include "GeoSacrificeBeamAbility.generated.h"

class ACharacter;

/**
 * Square special ability, phase 1 of the sacrifice pair (phase 2 = UGeoSacrificeDetonateAbility, same button):
 * channels a beam that marks every ally/neutral inside it as sacrificed (Status.Sacrificed via
 * SacrificeMarkEffect). While marked, a victim takes zero damage — the full amount is redirected, split equally
 * across the Square's alive walls + the Square, and accumulated into the CharacterAttributeSet's SacrificeValue.
 * Activation applies DetonateReadyEffect (Status.Square.DetonateReady), which blocks this channel from
 * re-activating and arms the detonate ability until it fires or the Square dies. The channel ends at
 * MaxChannelDuration or when the detonation cancels it. Cooldown commits at activation.
 */
UCLASS()
class GEOTRINITY_API UGeoSacrificeBeamAbility : public UGeoChannelBeamAbility
{
	GENERATED_BODY()

public:
	UGeoSacrificeBeamAbility();

	/**
	 * Server: captures damage about to be applied to a sacrificed victim.
	 * Returns true when the damage was fully redirected (the victim must then take none of it).
	 * Called from UGeoAttributeSetBase::PostGameplayEffectExecute before shield absorption.
	 */
	static bool TryRedirectIncomingDamage(UAbilitySystemComponent& VictimASC,
										  FGameplayEffectContextHandle const& DamageContext, float Damage);

protected:
	/** Arms the detonation (DetonateReadyEffect, server) and starts the channel. */
	virtual void Fire(FGeoAbilityTargetData const& AbilityTargetData) override;
	/** Removes all sacrifice marks (server) before calling Super. */
	virtual void EndAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
							FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
							bool bWasCancelled) override;

	/** Enforces MaxChannelDuration before delegating the beam scan to Super. */
	virtual void Tick(float DeltaTime) override;
	/** Server: marks entering allies/neutrals as sacrificed and unmarks leaving ones. */
	virtual void TickBeam(float DeltaTime, TArray<AActor*> const& ActorsInLine) override;

	virtual float GetCurrentBeamHalfWidth(ACharacter const* Character) const override { return BeamHalfWidth; }
	virtual uint8 GetScanAttitudeMask() const override { return TeamAttitudeMask::FriendlyOrNeutral; }

private:
	/**
	 * Server: adds the captured damage to the Square's SacrificeValue and splits it across its alive walls + the
	 * Square. Static on purpose: everything is derived from the mark GE (SquareASC, AbilityLevel), so redirection
	 * never needs a live channel instance. SourceAsc is the original damage dealer, kept as the source of the
	 * redirect shares so the Square's own damage stats never scale them.
	 */
	static void RedirectCapturedDamage(float Damage, UAbilitySystemComponent* SourceAsc,
									   UAbilitySystemComponent& SquareASC, int32 AbilityLevel);
	/** Server: removes the sacrifice mark GE from every tracked victim. */
	void RemoveAllSacrificeMarks();

	/** Beam half-width in cm used by the channel scan. */
	UPROPERTY(EditDefaultsOnly, Category = "Ability", meta = (ClampMin = "0", AllowPrivateAccess = true))
	float BeamHalfWidth = 100.f;

	/**
	 * Infinite GE applied to each sacrificed victim. Must grant Status.Sacrificed and carry a
	 * WhileActive GameplayCue so every client sees who is sacrificed.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Effects", meta = (AllowPrivateAccess = true))
	TInstancedStruct<FEffectData> SacrificeMarkEffect;

	/**
	 * Infinite GE applied to the Square on activation. Must grant Status.Square.DetonateReady; removed by the
	 * detonate ability (or death via the death-time effect purge).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Effects", meta = (AllowPrivateAccess = true))
	TInstancedStruct<FEffectData> DetonateReadyEffect;

	/** The channel ends (the detonation stays armed) after this many seconds. */
	UPROPERTY(EditDefaultsOnly, Category = "Ability", meta = (ClampMin = "0", AllowPrivateAccess = true))
	float MaxChannelDuration = 15.f;

	// Server-only bookkeeping: victim -> handle of its sacrifice mark GE.
	TMap<TWeakObjectPtr<AActor>, FActiveGameplayEffectHandle> SacrificedActors;
	float ChannelElapsed = 0.f;
};
