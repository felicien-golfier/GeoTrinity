// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/Damaging/GeoDamageGameplayAbility.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystem/GeoAscTypes.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "GeoTrinity/GeoTrinity.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoDamageGameplayAbility::CauseDamage(AActor* targetActor) const
{
	UGeoAbilitySystemLibrary::ApplyEffectFromDamageParams(MakeDamageEffectParamsFromClassDefaults(targetActor));
}

// ---------------------------------------------------------------------------------------------------------------------
float UGeoDamageGameplayAbility::GetDamageAtLevel_Implementation(int32 Level) const
{
	return DamageAmount.GetValueAtLevel(Level);
}

// ---------------------------------------------------------------------------------------------------------------------
float UGeoDamageGameplayAbility::GetDamage() const
{
	return GetDamageAtLevel(GetAbilityLevel());
}

// ---------------------------------------------------------------------------------------------------------------------
float UGeoDamageGameplayAbility::GetCooldown(int32 level) const
{
	float cooldown = 0.f;
	
	UGameplayEffect* pCooldownEffect = GetCooldownGameplayEffect();
	if(!pCooldownEffect)
		return cooldown;

	pCooldownEffect->DurationMagnitude.GetStaticMagnitudeIfPossible(level, cooldown);
	return cooldown;
}

// ---------------------------------------------------------------------------------------------------------------------
FDamageEffectParams UGeoDamageGameplayAbility::MakeDamageEffectParamsFromClassDefaults(AActor const* pTargetActor) const
{
	FDamageEffectParams params;
	params.WorldContextObject = GetAvatarActorFromActorInfo();
	params.DamageGameplayEffectClass = DamageEffectClass;
	params.SourceASC = GetAbilitySystemComponentFromActorInfo();
	params.TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(pTargetActor);
	params.AbilityLevel = GetAbilityLevel();
	params.BaseDamage = GetDamageAtLevel(params.AbilityLevel);
	params.StatusChance = ChanceToInflictStatus;
	params.StatusTag = StatusToInflictTag;
	params.DeathImpulseMagnitude = DeathImpulseMagnitude;
	params.KnockbackMagnitude = KnockbackMagnitude;
	params.KnockbackChancePercent = KnockbackChancePercent;
	params.bIsRadialDamage = bIsRadialDamage;
	if (bIsRadialDamage)
	{
		// params.RadialDamageOrigin = ?; // This needs to be set by the calling GA
		params.RadialDamageInnerRadius = RadialDamageInnerRadius;
		params.RadialDamageOuterRadius = RadialDamageOuterRadius;
	}
	
	return params;
}

// ---------------------------------------------------------------------------------------------------------------------
FVector UGeoDamageGameplayAbility::GetDirectionFromCauseToTarget(FDamageEffectParams const& damageEffectParams, const float pitchOverride/* = 0*/) const
{
	AActor const* pTargetActor = damageEffectParams.TargetASC ? damageEffectParams.TargetASC->GetAvatarActor() : nullptr;
	if (!pTargetActor)
		return FVector();

	FVector initalVector = pTargetActor->GetActorLocation() - GetAvatarActorFromActorInfo()->GetActorLocation();
	initalVector.Normalize();
	if (!FMath::IsNearlyZero(pitchOverride))
	{
		FRotator rotation = initalVector.Rotation();
		rotation.Pitch = pitchOverride;
		initalVector = rotation.Vector();
	}
	return initalVector;
}

// ---------------------------------------------------------------------------------------------------------------------
FVector UGeoDamageGameplayAbility::ComputeKnockbackVector(FVector const& originPoint, FVector const& destinationPoint, bool bPrintLog /*= false*/) const
{
	if (KnockbackChancePercent >= FMath::RandRange(1, 100))
	{
		if (bPrintLog) UE_LOG(LogGeoTrinity, Log, TEXT("UGeoDamageGameplayAbility::ComputeKnockbackVector SUCCESSFUL knockback"));
		FVector direction {destinationPoint - originPoint};
		direction.Z = 0;
		direction.Normalize();
		return direction * KnockbackMagnitude;
	}
	
	if (bPrintLog) UE_LOG(LogGeoTrinity, Log, TEXT("UGeoDamageGameplayAbility::ComputeKnockbackVector NO knockback"));
	return FVector::ZeroVector;
}

// ---------------------------------------------------------------------------------------------------------------------
FVector UGeoDamageGameplayAbility::ComputeDeathImpulseVector(FVector const& originPoint, FVector const& destinationPoint) const
{
	FVector direction {destinationPoint - originPoint};
	direction.Normalize();
	direction *= DeathImpulseMagnitude;
	
	UE_VLOG(this, LogGeoASC, VeryVerbose, TEXT("UGeoDamageGameplayAbility::ComputeDeathImpulseVector Death impulse: %s"), *direction.ToCompactString());
	
	return direction;
}
