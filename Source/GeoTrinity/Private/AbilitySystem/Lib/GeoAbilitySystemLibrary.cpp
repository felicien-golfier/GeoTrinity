// Fill out your copyright notice in the Description page of Project Settings.


// ReSharper disable CppUE4CodingStandardNamingViolationWarning
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystem/GeoAscTypes.h"
#include "AbilitySystem/Data/StatusInfo.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Settings/GameDataSettings.h"

// ---------------------------------------------------------------------------------------------------------------------
UAbilityInfo* UGeoAbilitySystemLibrary::GetAbilityInfo(UObject const* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}
		
	const UGameDataSettings* GDSettings = GetDefault<UGameDataSettings>();

	return GDSettings->GetLoadedDataAsset(GDSettings->AbilityInfo);	
}

// ---------------------------------------------------------------------------------------------------------------------
UStatusInfo* UGeoAbilitySystemLibrary::GetStatusInfo(UObject const* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}
		
	const UGameDataSettings* GDSettings = GetDefault<UGameDataSettings>();

	return GDSettings->GetLoadedDataAsset(GDSettings->StatusInfo);	
}

// ---------------------------------------------------------------------------------------------------------------------
FGameplayEffectContextHandle UGeoAbilitySystemLibrary::ApplyEffectFromDamageParams(FDamageEffectParams const& params)
{
	FGameplayEffectContextHandle contextHandle;
	UGeoAbilitySystemComponent* pSourceASC = Cast<UGeoAbilitySystemComponent>(params.SourceASC);
	checkf(pSourceASC, TEXT("AbilitySystemLibrary::ApplyEffectFromDamageParams: needs a valid Source ASC to apply effect"))
	if(!IsValid(pSourceASC) || !IsValid(params.TargetASC))
		return contextHandle;
	checkf(params.DamageGameplayEffectClass, TEXT("AbilitySystemLibrary::ApplyEffectFromDamageParams: Damage effect class not set"))
	
	contextHandle = pSourceASC->MakeEffectContext();
	contextHandle.AddSourceObject(pSourceASC->GetAvatarActor());
	
	FGeoGameplayEffectContext* pGeoEffectContext = static_cast<FGeoGameplayEffectContext*>(contextHandle.Get());
	checkf(pGeoEffectContext, TEXT("AbilitySystemLibrary::ApplyEffectFromDamageParams: Failed to create GeoEffectContext"))
	
	/** Knockbacks **/
	pGeoEffectContext->SetDeathImpulseVector(params.DeathImpulseVector);
	pGeoEffectContext->SetKnockbackVector(params.KnockbackVector);

	/** Radial damage **/
	pGeoEffectContext->SetIsRadialDamage(params.bIsRadialDamage);
	pGeoEffectContext->SetRadialDamageInnerRadius(params.RadialDamageInnerRadius);
	pGeoEffectContext->SetRadialDamageOuterRadius(params.RadialDamageOuterRadius);
	pGeoEffectContext->SetRadialDamageOrigin(params.RadialDamageOrigin);
	
	FGameplayEffectSpecHandle specHandle = params.SourceASC->MakeOutgoingSpec(params.DamageGameplayEffectClass, params.AbilityLevel,
	contextHandle);
	
	/** Type of damage **/
	FGeoGameplayTags const& tags = FGeoGameplayTags::Get();
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(specHandle, tags.Gameplay_Damage, params.BaseDamage);

	/** APPLY EFFECT **/
	params.TargetASC->ApplyGameplayEffectSpecToSelf(*specHandle.Data);
	
	/** Status **/
	if (FMath::RandRange(1, 100) <= params.StatusChance)
	{
		ApplyStatusToTarget(params.TargetASC, pSourceASC, params.StatusTag, params.AbilityLevel);
	}

	return contextHandle;
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoAbilitySystemLibrary::ApplyStatusToTarget(UAbilitySystemComponent* pTargetASC,
	UAbilitySystemComponent* pSourceASC, FGameplayTag const& statusTag, int32 level)
{
	if (!pTargetASC || !pSourceASC)
		return false;

	const UGameDataSettings* GDSettings = GetDefault<UGameDataSettings>();

	UStatusInfo const* pStatusInfos = GDSettings->GetLoadedDataAsset(GDSettings->StatusInfo);
	if (!pStatusInfos)
		return false;

	// INFOS
	FRpgStatusInfo statusInfo;
	if (!pStatusInfos->FillStatusInfoFromTag(statusTag, statusInfo))
		return false;
	
	if (!IsValid(statusInfo.StatusEffect))
	{
		UE_VLOG(pSourceASC, LogGeoASC, VeryVerbose, TEXT("No valid effect registered for Status %s"), *statusInfo.StatusDisplayName);
		return false;
	}

	FGameplayEffectContextHandle contextHandle = pSourceASC->MakeEffectContext();
	contextHandle.AddSourceObject(pSourceASC->GetAvatarActor());
	
	FGameplayEffectSpecHandle specHandle = pSourceASC->MakeOutgoingSpec(statusInfo.StatusEffect, level,
		contextHandle);

	pSourceASC->ApplyGameplayEffectSpecToTarget(*specHandle.Data, pTargetASC);
	
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
FGameplayTag UGeoAbilitySystemLibrary::GetGameplayTagFromRootTagString(FString const& StringOfTag)
{
	static TMap<FString, FGameplayTag> RootTagNameToTagMap {};
	if (!RootTagNameToTagMap.Find(StringOfTag))
	{
		RootTagNameToTagMap.Add(StringOfTag, FGameplayTag::RequestGameplayTag(FName(StringOfTag)));
	}
	return RootTagNameToTagMap[StringOfTag];
}

// ---------------------------------------------------------------------------------------------------------------------
FGameplayTag UGeoAbilitySystemLibrary::GetAbilityTagFromSpec(FGameplayAbilitySpec const& Spec)
{
	if (!Spec.Ability)
		return FGameplayTag();

	return GetAbilityTagFromAbility(*Spec.Ability.Get());
}

// ---------------------------------------------------------------------------------------------------------------------
FGameplayTag UGeoAbilitySystemLibrary::GetAbilityTagFromAbility(UGameplayAbility const& Ability)
{
	FGameplayTag const tagToMatch = GetGameplayTagFromRootTagString(RootTagNames::AbilityIDTag);
	for (FGameplayTag const& currentTag : Ability.GetAssetTags())
	{
		if (currentTag.MatchesTag(tagToMatch))
		{
			return currentTag;
		}
	}
	return FGameplayTag();
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoAbilitySystemLibrary::IsBlockedHit(FGameplayEffectContextHandle const& effectContextHandle)
{
	FGeoGameplayEffectContext const* pGeoContext = static_cast<FGeoGameplayEffectContext const*>(effectContextHandle.Get());

	return pGeoContext ? pGeoContext->IsBlockedHit() : false;
}

bool UGeoAbilitySystemLibrary::IsCriticalHit(FGameplayEffectContextHandle const& effectContextHandle)
{
	FGeoGameplayEffectContext const* pGeoContext = static_cast<FGeoGameplayEffectContext const*>(effectContextHandle.Get());
	
	return pGeoContext ? pGeoContext->IsCriticalHit() : false;
}

bool UGeoAbilitySystemLibrary::IsSuccessfulDebuff(FGameplayEffectContextHandle const& effectContextHandle)
{
	FGeoGameplayEffectContext const* pGeoContext = static_cast<FGeoGameplayEffectContext const*>(effectContextHandle.Get());

	return pGeoContext ? pGeoContext->GetStatusTag().IsValid() : false;
}

float UGeoAbilitySystemLibrary::GetDebuffDamage(FGameplayEffectContextHandle const& effectContextHandle)
{
	FGeoGameplayEffectContext const* pGeoContext = static_cast<FGeoGameplayEffectContext const*>(effectContextHandle.Get());
	
	return pGeoContext ? pGeoContext->GetDebuffDamage() : 0.f;
}

float UGeoAbilitySystemLibrary::GetDebuffDuration(FGameplayEffectContextHandle const& effectContextHandle)
{
	FGeoGameplayEffectContext const* pGeoContext = static_cast<FGeoGameplayEffectContext const*>(effectContextHandle.Get());
	
	return pGeoContext ? pGeoContext->GetDebuffDamage() : 0.f;
}

float UGeoAbilitySystemLibrary::GetDebuffFrequency(FGameplayEffectContextHandle const& effectContextHandle)
{
	FGeoGameplayEffectContext const* pGeoContext = static_cast<FGeoGameplayEffectContext const*>(effectContextHandle.Get());
	
	return pGeoContext ? pGeoContext->GetDebuffFrequency() : 0.f;
}

FVector UGeoAbilitySystemLibrary::GetDeathImpulseVector(FGameplayEffectContextHandle const& effectContextHandle)
{
	FGeoGameplayEffectContext const* pGeoContext = static_cast<FGeoGameplayEffectContext const*>(effectContextHandle.Get());
	
	return pGeoContext ? pGeoContext->GetDeathImpulseVector() : FVector::ZeroVector;
}

FVector UGeoAbilitySystemLibrary::GetKnockbackVector(FGameplayEffectContextHandle const& effectContextHandle)
{
	FGeoGameplayEffectContext const* pGeoContext = static_cast<FGeoGameplayEffectContext const*>(effectContextHandle.Get());
	
	return pGeoContext ? pGeoContext->GetKnockbackVector() : FVector::ZeroVector;
}

bool UGeoAbilitySystemLibrary::GetIsRadialDamage(FGameplayEffectContextHandle const& effectContextHandle)
{
	FGeoGameplayEffectContext const* pGeoContext = static_cast<FGeoGameplayEffectContext const*>(effectContextHandle.Get());
	
	return pGeoContext ? pGeoContext->GetIsRadialDamage() : false;
}

float UGeoAbilitySystemLibrary::GetRadialDamageInnerRadius(FGameplayEffectContextHandle const& effectContextHandle)
{
	FGeoGameplayEffectContext const* pGeoContext = static_cast<FGeoGameplayEffectContext const*>(effectContextHandle.Get());
	
	return pGeoContext ? pGeoContext->GetRadialDamageInnerRadius() : 0.f;
}

float UGeoAbilitySystemLibrary::GetRadialDamageOuterRadius(FGameplayEffectContextHandle const& effectContextHandle)
{
	FGeoGameplayEffectContext const* pGeoContext = static_cast<FGeoGameplayEffectContext const*>(effectContextHandle.Get());
	
	return pGeoContext ? pGeoContext->GetRadialDamageOuterRadius() : 0.f;
}

FVector UGeoAbilitySystemLibrary::GetRadialDamageOrigin(FGameplayEffectContextHandle const& effectContextHandle)
{
	FGeoGameplayEffectContext const* pGeoContext = static_cast<FGeoGameplayEffectContext const*>(effectContextHandle.Get());
	
	return pGeoContext ? pGeoContext->GetRadialDamageOrigin() : FVector::ZeroVector;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySystemLibrary::SetIsBlockedHit(FGameplayEffectContextHandle& effectContextHandle,
	const bool bIsBlockedHit)
{
	FGeoGameplayEffectContext * pGeoContext = static_cast<FGeoGameplayEffectContext *>(effectContextHandle.Get());
	if (pGeoContext)
	{
		pGeoContext->SetIsBlockedHit(bIsBlockedHit);
	}
}

void UGeoAbilitySystemLibrary::SetIsCriticalHit(FGameplayEffectContextHandle& effectContextHandle,
	const bool bIsCriticalHit)
{
	FGeoGameplayEffectContext * pGeoContext = static_cast<FGeoGameplayEffectContext *>(effectContextHandle.Get());
	if (pGeoContext)
	{
		pGeoContext->SetIsCriticalHit(bIsCriticalHit);
	}
}

void UGeoAbilitySystemLibrary::SetStatusTag(FGameplayEffectContextHandle& effectContextHandle,
	const FGameplayTag statusTag)
{
	FGeoGameplayEffectContext * pGeoContext = static_cast<FGeoGameplayEffectContext *>(effectContextHandle.Get());
	if (pGeoContext)
	{
		pGeoContext->SetStatusTag(statusTag);
	}
}

void UGeoAbilitySystemLibrary::SetDebuffDamage(FGameplayEffectContextHandle& effectContextHandle,
	const float debuffDamage)
{
	FGeoGameplayEffectContext * pGeoContext = static_cast<FGeoGameplayEffectContext *>(effectContextHandle.Get());
	if (pGeoContext)
	{
		pGeoContext->SetDebuffDamage(debuffDamage);
	}
}

void UGeoAbilitySystemLibrary::SetDebuffFrequency(FGameplayEffectContextHandle& effectContextHandle,
	const float debuffFrequency)
{
	FGeoGameplayEffectContext * pGeoContext = static_cast<FGeoGameplayEffectContext *>(effectContextHandle.Get());
	if (pGeoContext)
	{
		pGeoContext->SetDebuffFrequency(debuffFrequency);
	}
}

void UGeoAbilitySystemLibrary::SetDebuffDuration(FGameplayEffectContextHandle& effectContextHandle,
	const float debuffDuration)
{
	FGeoGameplayEffectContext * pGeoContext = static_cast<FGeoGameplayEffectContext *>(effectContextHandle.Get());
	if (pGeoContext)
	{
		pGeoContext->SetDebuffDuration(debuffDuration);
	}
}

void UGeoAbilitySystemLibrary::SetDeathImpulseVector(FGameplayEffectContextHandle& effectContextHandle,
	FVector const& inVector)
{
	FGeoGameplayEffectContext * pGeoContext = static_cast<FGeoGameplayEffectContext *>(effectContextHandle.Get());
	if (pGeoContext)
	{
		pGeoContext->SetDeathImpulseVector(inVector);
	}
}

void UGeoAbilitySystemLibrary::SetKnockbackVector(FGameplayEffectContextHandle& effectContextHandle,
	FVector const& inVector)
{
	FGeoGameplayEffectContext * pGeoContext = static_cast<FGeoGameplayEffectContext *>(effectContextHandle.Get());
	if (pGeoContext)
	{
		pGeoContext->SetKnockbackVector(inVector);
	}
}

void UGeoAbilitySystemLibrary::SetIsRadialDamage(FGameplayEffectContextHandle& effectContextHandle,
	const bool bIsRadialDamage)
{
	FGeoGameplayEffectContext * pGeoContext = static_cast<FGeoGameplayEffectContext *>(effectContextHandle.Get());
	if (pGeoContext)
	{
		pGeoContext->SetIsRadialDamage(bIsRadialDamage);
	}
}

void UGeoAbilitySystemLibrary::SetRadialDamageInnerRadius(FGameplayEffectContextHandle& effectContextHandle,
	const float radialDamageInnerRadius)
{
	FGeoGameplayEffectContext * pGeoContext = static_cast<FGeoGameplayEffectContext *>(effectContextHandle.Get());
	if (pGeoContext)
	{
		pGeoContext->SetRadialDamageInnerRadius(radialDamageInnerRadius);
	}
}

void UGeoAbilitySystemLibrary::SetRadialDamageOuterRadius(FGameplayEffectContextHandle& effectContextHandle,
	const float radialDamageOuterRadius)
{
	FGeoGameplayEffectContext * pGeoContext = static_cast<FGeoGameplayEffectContext *>(effectContextHandle.Get());
	if (pGeoContext)
	{
		pGeoContext->SetRadialDamageOuterRadius(radialDamageOuterRadius);
	}
}

void UGeoAbilitySystemLibrary::SetRadialDamageOrigin(FGameplayEffectContextHandle& effectContextHandle,
	FVector const& inVector)
{
	FGeoGameplayEffectContext * pGeoContext = static_cast<FGeoGameplayEffectContext *>(effectContextHandle.Get());
	if (pGeoContext)
	{
		pGeoContext->SetRadialDamageOrigin(inVector);
	}
}