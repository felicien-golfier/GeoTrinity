#pragma once

#include "CoreMinimal.h"
#include "GeoAttributeSetBase.h"

#include "CharacterAttributeSet.generated.h"

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

	// Shield
	UPROPERTY(BlueprintReadOnly, Category = "Shield", ReplicatedUsing = OnRep_Shield)
	FGameplayAttributeData Shield;
	ATTRIBUTE_ACCESSORS(UCharacterAttributeSet, Shield)

	UPROPERTY(BlueprintReadOnly, Category = "Shield", ReplicatedUsing = OnRep_MaxShield)
	FGameplayAttributeData MaxShield;
	ATTRIBUTE_ACCESSORS(UCharacterAttributeSet, MaxShield)

	// Multipliers
	UPROPERTY(BlueprintReadOnly, Category = "Multiplier", ReplicatedUsing = OnRep_HealingPower)
	FGameplayAttributeData HealingPower;
	ATTRIBUTE_ACCESSORS(UCharacterAttributeSet, HealingPower)

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
	void OnRep_Shield(FGameplayAttributeData const& OldShield);
	UFUNCTION()
	void OnRep_MaxShield(FGameplayAttributeData const& OldMaxShield);
	UFUNCTION()
	void OnRep_HealingPower(FGameplayAttributeData const& OldHealingPower);
	UFUNCTION()
	void OnRep_DamageReduction(FGameplayAttributeData const& OldDamageReduction);
	UFUNCTION()
	void OnRep_MovementSpeedMultiplier(FGameplayAttributeData const& OldMovementSpeedMultiplier);
	UFUNCTION()
	void OnRep_RotationSpeedMultiplier(FGameplayAttributeData const& OldRotationSpeedMultiplier);
};
