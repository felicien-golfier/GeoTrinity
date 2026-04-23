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
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Triangle: ammo system
	UPROPERTY(BlueprintReadOnly, Category = "Ammo", ReplicatedUsing = OnRep_Ammo)
	FGameplayAttributeData Ammo;
	ATTRIBUTE_ACCESSORS(UCharacterAttributeSet, Ammo)

	UPROPERTY(BlueprintReadOnly, Category = "Ammo", ReplicatedUsing = OnRep_MaxAmmo)
	FGameplayAttributeData MaxAmmo;
	ATTRIBUTE_ACCESSORS(UCharacterAttributeSet, MaxAmmo)

	// Multipliers
	UPROPERTY(BlueprintReadOnly, Category = "Multiplier", ReplicatedUsing = OnRep_HealMultiplier)
	FGameplayAttributeData HealMultiplier;
	ATTRIBUTE_ACCESSORS(UCharacterAttributeSet, HealMultiplier)

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

protected:
	UFUNCTION()
	void OnRep_Ammo(FGameplayAttributeData const& OldAmmo);
	UFUNCTION()
	void OnRep_MaxAmmo(FGameplayAttributeData const& OldMaxAmmo);
	UFUNCTION()
	void OnRep_HealMultiplier(FGameplayAttributeData const& OldHealMultiplier);
	UFUNCTION()
	void OnRep_DamageMultiplier(FGameplayAttributeData const& OldDamageMultiplier);
	UFUNCTION()
	void OnRep_DamageReduction(FGameplayAttributeData const& OldDamageReduction);
	UFUNCTION()
	void OnRep_MovementSpeedMultiplier(FGameplayAttributeData const& OldMovementSpeedMultiplier);
	UFUNCTION()
	void OnRep_RotationSpeedMultiplier(FGameplayAttributeData const& OldRotationSpeedMultiplier);
};
