// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GeoAttributeSetBase.h"

#include "CharacterAttributeSet.generated.h"

/**
 * Extends UGeoAttributeSetBase with player-specific attributes: ammo, heal/damage/movement multipliers.
 * Used by APlayableCharacter's ASC.
 */
UCLASS()
class GEOTRINITY_API UCharacterAttributeSet : public UGeoAttributeSetBase
{
	GENERATED_BODY()

public:
	/** Registers all player-specific attributes (ammo, multipliers, SacrificeValue) for replication. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Triangle: ammo system
	UPROPERTY(BlueprintReadOnly, Category = "GeoAmmo", ReplicatedUsing = OnRep_Ammo)
	FGameplayAttributeData Ammo;
	ATTRIBUTE_ACCESSORS(UCharacterAttributeSet, Ammo)

	UPROPERTY(BlueprintReadOnly, Category = "GeoAmmo", ReplicatedUsing = OnRep_MaxAmmo)
	FGameplayAttributeData MaxAmmo;
	ATTRIBUTE_ACCESSORS(UCharacterAttributeSet, MaxAmmo)

	// Multipliers
	UPROPERTY(BlueprintReadOnly, Category = "GeoMultiplier", ReplicatedUsing = OnRep_AppliedHealBoost)
	FGameplayAttributeData AppliedHealBoost;
	ATTRIBUTE_ACCESSORS(UCharacterAttributeSet, AppliedHealBoost)

	UPROPERTY(BlueprintReadOnly, Category = "GeoMultiplier", ReplicatedUsing = OnRep_ReceivedHealBoost)
	FGameplayAttributeData ReceivedHealBoost;
	ATTRIBUTE_ACCESSORS(UCharacterAttributeSet, ReceivedHealBoost)

	UPROPERTY(BlueprintReadOnly, Category = "GeoMultiplier", ReplicatedUsing = OnRep_DamageMultiplier)
	FGameplayAttributeData DamageMultiplier;
	ATTRIBUTE_ACCESSORS(UCharacterAttributeSet, DamageMultiplier)

	UPROPERTY(BlueprintReadOnly, Category = "GeoMultiplier", ReplicatedUsing = OnRep_DamageReduction)
	FGameplayAttributeData DamageReduction;
	ATTRIBUTE_ACCESSORS(UCharacterAttributeSet, DamageReduction)

	UPROPERTY(BlueprintReadOnly, Category = "GeoMultiplier", ReplicatedUsing = OnRep_MovementSpeedMultiplier)
	FGameplayAttributeData MovementSpeedMultiplier;
	ATTRIBUTE_ACCESSORS(UCharacterAttributeSet, MovementSpeedMultiplier)


	/** Square: damage captured by the sacrifice beam, consumed by the sacrifice detonation. Replicated so the HUD can
	 * display the armed value. */
	UPROPERTY(BlueprintReadOnly, Category = "GeoSacrifice", ReplicatedUsing = OnRep_SacrificeValue)
	FGameplayAttributeData SacrificeValue;
	ATTRIBUTE_ACCESSORS(UCharacterAttributeSet, SacrificeValue)

	/** Circle: healing recorded since the sweet-spot passive's gauge was last consumed; a full gauge grants the charge
	 * beam's next sweet-spot release the passive's damage-multiplier boost. Replicated so the HUD status-bar gauge
	 * shows the fill. */
	UPROPERTY(BlueprintReadOnly, Category = "GeoHealCharge", ReplicatedUsing = OnRep_HealCharge)
	FGameplayAttributeData HealCharge;
	ATTRIBUTE_ACCESSORS(UCharacterAttributeSet, HealCharge)

protected:
	/** Notifies the GAS aggregator that Ammo changed so dependent calculations re-evaluate. */
	UFUNCTION()
	void OnRep_Ammo(FGameplayAttributeData const& OldAmmo);
	/** Notifies the GAS aggregator that MaxAmmo changed so dependent calculations re-evaluate. */
	UFUNCTION()
	void OnRep_MaxAmmo(FGameplayAttributeData const& OldMaxAmmo);
	/** Notifies the GAS aggregator that AppliedHealBoost changed so dependent calculations re-evaluate. */
	UFUNCTION()
	void OnRep_AppliedHealBoost(FGameplayAttributeData const& OldAppliedHealBoost);
	/** Notifies the GAS aggregator that ReceivedHealBoost changed so dependent calculations re-evaluate. */
	UFUNCTION()
	void OnRep_ReceivedHealBoost(FGameplayAttributeData const& OldReceivedHealBoost);
	/** Notifies the GAS aggregator that DamageMultiplier changed so dependent calculations re-evaluate. */
	UFUNCTION()
	void OnRep_DamageMultiplier(FGameplayAttributeData const& OldDamageMultiplier);
	/** Notifies the GAS aggregator that DamageReduction changed so dependent calculations re-evaluate. */
	UFUNCTION()
	void OnRep_DamageReduction(FGameplayAttributeData const& OldDamageReduction);
	/** Notifies the GAS aggregator that MovementSpeedMultiplier changed so UGeoCharacterMovementComponent re-evaluates speed. */
	UFUNCTION()
	void OnRep_MovementSpeedMultiplier(FGameplayAttributeData const& OldMovementSpeedMultiplier);

	/** Notifies the GAS aggregator that SacrificeValue changed so the HUD can display the updated armed value. */
	UFUNCTION()
	void OnRep_SacrificeValue(FGameplayAttributeData const& OldSacrificeValue);
	/** Notifies the GAS aggregator that HealCharge changed so the HUD gauge can update its fill. */
	UFUNCTION()
	void OnRep_HealCharge(FGameplayAttributeData const& OldHealCharge);
};
