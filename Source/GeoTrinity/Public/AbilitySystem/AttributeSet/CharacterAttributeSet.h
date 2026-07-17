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
	UPROPERTY(BlueprintReadOnly, Category = "Ammo", ReplicatedUsing = OnRep_Ammo)
	FGameplayAttributeData Ammo;
	ATTRIBUTE_ACCESSORS(UCharacterAttributeSet, Ammo)

	UPROPERTY(BlueprintReadOnly, Category = "Ammo", ReplicatedUsing = OnRep_MaxAmmo)
	FGameplayAttributeData MaxAmmo;
	ATTRIBUTE_ACCESSORS(UCharacterAttributeSet, MaxAmmo)

	// Multipliers
	UPROPERTY(BlueprintReadOnly, Category = "Multiplier", ReplicatedUsing = OnRep_AppliedHealBoost)
	FGameplayAttributeData AppliedHealBoost;
	ATTRIBUTE_ACCESSORS(UCharacterAttributeSet, AppliedHealBoost)

	UPROPERTY(BlueprintReadOnly, Category = "Multiplier", ReplicatedUsing = OnRep_ReceivedHealBoost)
	FGameplayAttributeData ReceivedHealBoost;
	ATTRIBUTE_ACCESSORS(UCharacterAttributeSet, ReceivedHealBoost)

	UPROPERTY(BlueprintReadOnly, Category = "Multiplier", ReplicatedUsing = OnRep_DamageMultiplier)
	FGameplayAttributeData DamageMultiplier;
	ATTRIBUTE_ACCESSORS(UCharacterAttributeSet, DamageMultiplier)

	UPROPERTY(BlueprintReadOnly, Category = "Multiplier", ReplicatedUsing = OnRep_DamageReduction)
	FGameplayAttributeData DamageReduction;
	ATTRIBUTE_ACCESSORS(UCharacterAttributeSet, DamageReduction)

	UPROPERTY(BlueprintReadOnly, Category = "Multiplier", ReplicatedUsing = OnRep_MovementSpeedMultiplier)
	FGameplayAttributeData MovementSpeedMultiplier;
	ATTRIBUTE_ACCESSORS(UCharacterAttributeSet, MovementSpeedMultiplier)

	UPROPERTY(BlueprintReadOnly, Category = "Multiplier", ReplicatedUsing = OnRep_RotationSpeedMultiplier)
	FGameplayAttributeData RotationSpeedMultiplier;
	ATTRIBUTE_ACCESSORS(UCharacterAttributeSet, RotationSpeedMultiplier)

	/** Square: damage captured by the sacrifice beam, consumed by the sacrifice detonation. Replicated so the HUD can
	 * display the armed value. */
	UPROPERTY(BlueprintReadOnly, Category = "Sacrifice", ReplicatedUsing = OnRep_SacrificeValue)
	FGameplayAttributeData SacrificeValue;
	ATTRIBUTE_ACCESSORS(UCharacterAttributeSet, SacrificeValue)

	/** Circle: healing recorded since the sweet-spot passive's gauge was last consumed; a full gauge grants the charge
	 * beam's next sweet-spot release the passive's damage-multiplier boost. Replicated so the HUD status-bar gauge shows
	 * the fill. */
	UPROPERTY(BlueprintReadOnly, Category = "HealCharge", ReplicatedUsing = OnRep_HealCharge)
	FGameplayAttributeData HealCharge;
	ATTRIBUTE_ACCESSORS(UCharacterAttributeSet, HealCharge)

protected:
	UFUNCTION()
	void OnRep_Ammo(FGameplayAttributeData const& OldAmmo);
	UFUNCTION()
	void OnRep_MaxAmmo(FGameplayAttributeData const& OldMaxAmmo);
	UFUNCTION()
	void OnRep_AppliedHealBoost(FGameplayAttributeData const& OldAppliedHealBoost);
	UFUNCTION()
	void OnRep_ReceivedHealBoost(FGameplayAttributeData const& OldReceivedHealBoost);
	UFUNCTION()
	void OnRep_DamageMultiplier(FGameplayAttributeData const& OldDamageMultiplier);
	UFUNCTION()
	void OnRep_DamageReduction(FGameplayAttributeData const& OldDamageReduction);
	UFUNCTION()
	void OnRep_MovementSpeedMultiplier(FGameplayAttributeData const& OldMovementSpeedMultiplier);
	UFUNCTION()
	void OnRep_RotationSpeedMultiplier(FGameplayAttributeData const& OldRotationSpeedMultiplier);
	UFUNCTION()
	void OnRep_SacrificeValue(FGameplayAttributeData const& OldSacrificeValue);
	UFUNCTION()
	void OnRep_HealCharge(FGameplayAttributeData const& OldHealCharge);
};
