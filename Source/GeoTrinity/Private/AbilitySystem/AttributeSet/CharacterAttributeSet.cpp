#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"

#include "Net/UnrealNetwork.h"

// -----------------------------------------------------------------------------------------------------------------------------------------
void UCharacterAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// For GAS attributes, REPNOTIFY_Always is the standard pattern because the prediction system needs OnRep to fire
	// even on unchanged values to handle prediction rollbacks correctly

	DOREPLIFETIME_CONDITION_NOTIFY(UCharacterAttributeSet, Ammo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCharacterAttributeSet, MaxAmmo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCharacterAttributeSet, Shield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCharacterAttributeSet, MaxShield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCharacterAttributeSet, HealingPower, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCharacterAttributeSet, DamageReduction, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCharacterAttributeSet, MovementSpeedMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCharacterAttributeSet, RotationSpeedMultiplier, COND_None, REPNOTIFY_Always);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UCharacterAttributeSet::OnRep_Ammo(FGameplayAttributeData const& OldAmmo)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCharacterAttributeSet, Ammo, OldAmmo);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UCharacterAttributeSet::OnRep_MaxAmmo(FGameplayAttributeData const& OldMaxAmmo)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCharacterAttributeSet, MaxAmmo, OldMaxAmmo);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UCharacterAttributeSet::OnRep_Shield(FGameplayAttributeData const& OldShield)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCharacterAttributeSet, Shield, OldShield);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UCharacterAttributeSet::OnRep_MaxShield(FGameplayAttributeData const& OldMaxShield)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCharacterAttributeSet, MaxShield, OldMaxShield);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UCharacterAttributeSet::OnRep_HealingPower(FGameplayAttributeData const& OldHealingPower)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCharacterAttributeSet, HealingPower, OldHealingPower);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UCharacterAttributeSet::OnRep_DamageReduction(FGameplayAttributeData const& OldDamageReduction)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCharacterAttributeSet, DamageReduction, OldDamageReduction);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UCharacterAttributeSet::OnRep_MovementSpeedMultiplier(FGameplayAttributeData const& OldMovementSpeedMultiplier)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCharacterAttributeSet, MovementSpeedMultiplier, OldMovementSpeedMultiplier);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UCharacterAttributeSet::OnRep_RotationSpeedMultiplier(FGameplayAttributeData const& OldRotationSpeedMultiplier)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCharacterAttributeSet, RotationSpeedMultiplier, OldRotationSpeedMultiplier);
}
