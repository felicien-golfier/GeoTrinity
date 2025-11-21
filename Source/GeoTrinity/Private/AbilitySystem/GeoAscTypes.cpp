// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/GeoAscTypes.h"

namespace 
{
enum RepFlag
{
	REP_IsBlockedHit,
	REP_IsCriticalHit,
	REP_DebuffDamage,
	REP_DebuffDuration,
	REP_DebuffFrequency,
	REP_DeathImpulseVector,
	REP_KnockbackVector,
	REP_IsRadialDamage,
	REP_RadialDamageInnerRadius,
	REP_RadialDamageOuterRadius,
	REP_RadialDamageOrigin,
	REP_MAX
 };
}

FGeoGameplayEffectContext* FGeoGameplayEffectContext::Duplicate() const
{
	FGeoGameplayEffectContext* NewContext = new FGeoGameplayEffectContext();
	*NewContext = *this;
	if (GetHitResult())
	{
		// Does a deep copy of the hit result
		NewContext->AddHitResult(*GetHitResult(), true);
	}
	return NewContext;
}

bool FGeoGameplayEffectContext::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	FGameplayEffectContext::NetSerialize(Ar, Map, bOutSuccess);
	
	uint32 RepBits = 0;
	
	if (Ar.IsSaving())
	{
		if (bIsBlockedHit)
		{
			RepBits |= 1 << REP_IsBlockedHit;
		}
		if (bIsCriticalHit)
		{
			RepBits |= 1 << REP_IsCriticalHit;
		}
		if (GetIsSuccessfulDebuff())
		{
			if (!FMath::IsNearlyZero(DebuffDamage))
			{
				RepBits |= 1 << REP_DebuffDamage;
			}
			if (!FMath::IsNearlyZero(DebuffDuration))
			{
				RepBits |= 1 << REP_DebuffDuration;
			}
			if (!FMath::IsNearlyZero(DebuffFrequency))
			{
				RepBits |= 1 << REP_DebuffFrequency;
			}
		}
		if (!DeathImpulseVector.IsZero())
		{
			RepBits |= 1 << REP_DeathImpulseVector;
		}
		if (!KnockbackVector.IsZero())
		{
			RepBits |= 1 << REP_KnockbackVector;
		}
		
		if (bIsRadialDamage)
		{
			RepBits |= 1 << REP_IsRadialDamage;
			if (!FMath::IsNearlyZero(RadialDamageInnerRadius))
			{
				RepBits |= 1 << REP_RadialDamageInnerRadius;
			}
			if (!FMath::IsNearlyZero(RadialDamageOuterRadius))
			{
				RepBits |= 1 << REP_RadialDamageOuterRadius;
			}
			if (!RadialDamageOrigin.IsZero())
			{
				RepBits |= 1 << REP_RadialDamageOrigin;
			}
		}
	}

	Ar.SerializeBits(&RepBits, REP_MAX);

	// For booleans only (no need to use the archive)
	if (Ar.IsLoading())
	{
		bIsBlockedHit		= RepBits & (1 & REP_IsBlockedHit);
		bIsCriticalHit		= RepBits & (1 & REP_IsCriticalHit);
		bIsRadialDamage		= RepBits & (1 & REP_IsRadialDamage);
		// Need to load the status tag to check if status data is present
		Ar << StatusTag;
	}

	// Serialization in both cases (check RepBits to see whether we need to save data, or read RepBits to see what's in the archive to load)
	if (GetIsSuccessfulDebuff())
	{
		if (RepBits & (1 & REP_DebuffDamage))
		{
			Ar << DebuffDamage;
		}
		if (RepBits & (1 & REP_DebuffDuration))
		{
			Ar << DebuffDuration;
		}
		if (RepBits & (1 & REP_DebuffFrequency))
		{
			Ar << DebuffFrequency;
		}
	}
	if (RepBits & (1 & REP_DeathImpulseVector))
	{
		DeathImpulseVector.NetSerialize(Ar, Map, bOutSuccess);
	}
	if (RepBits & (1 & REP_KnockbackVector))
	{
		KnockbackVector.NetSerialize(Ar, Map, bOutSuccess);
	}
	if (bIsRadialDamage)
	{
		if (RepBits & (1 & REP_RadialDamageInnerRadius))
		{
			Ar << RadialDamageInnerRadius;
		}
		if (RepBits & (1 & REP_RadialDamageOuterRadius))
		{
			Ar << RadialDamageOuterRadius;
		}
		if (RepBits & (1 & REP_RadialDamageOrigin))
		{
			RadialDamageOrigin.NetSerialize(Ar, Map, bOutSuccess);
		}
	}
	
	bOutSuccess = true;
	return true;
}