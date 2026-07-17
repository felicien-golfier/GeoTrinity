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
	DOREPLIFETIME_CONDITION_NOTIFY(UCharacterAttributeSet, AppliedHealBoost, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCharacterAttributeSet, ReceivedHealBoost, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCharacterAttributeSet, DamageMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCharacterAttributeSet, DamageReduction, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCharacterAttributeSet, MovementSpeedMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCharacterAttributeSet, RotationSpeedMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCharacterAttributeSet, SacrificeValue, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCharacterAttributeSet, HealCharge, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCharacterAttributeSet, HealChargeStartTime, COND_None, REPNOTIFY_Always);
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
void UCharacterAttributeSet::OnRep_AppliedHealBoost(FGameplayAttributeData const& OldAppliedHealBoost)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCharacterAttributeSet, AppliedHealBoost, OldAppliedHealBoost);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UCharacterAttributeSet::OnRep_ReceivedHealBoost(FGameplayAttributeData const& OldReceivedHealBoost)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCharacterAttributeSet, ReceivedHealBoost, OldReceivedHealBoost);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UCharacterAttributeSet::OnRep_DamageMultiplier(FGameplayAttributeData const& OldDamageMultiplier)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCharacterAttributeSet, DamageMultiplier, OldDamageMultiplier);
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

// -----------------------------------------------------------------------------------------------------------------------------------------
void UCharacterAttributeSet::OnRep_SacrificeValue(FGameplayAttributeData const& OldSacrificeValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCharacterAttributeSet, SacrificeValue, OldSacrificeValue);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UCharacterAttributeSet::OnRep_HealCharge(FGameplayAttributeData const& OldHealCharge)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCharacterAttributeSet, HealCharge, OldHealCharge);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UCharacterAttributeSet::OnRep_HealChargeStartTime(FGameplayAttributeData const& OldHealChargeStartTime)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCharacterAttributeSet, HealChargeStartTime, OldHealChargeStartTime);
}
