// Fill out your copyright notice in the Description page of Project Settings.

// ReSharper disable CppUE4CodingStandardNamingViolationWarning
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"

#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystem/Data/StatusInfo.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystem/GeoAscTypes.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerState.h"
#include "GameplayEffectTypes.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Settings/GameDataSettings.h"
#include "StructUtils/InstancedStruct.h"

// ---------------------------------------------------------------------------------------------------------------------
UAbilityInfo* UGeoAbilitySystemLibrary::GetAbilityInfo(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	const UGameDataSettings* GDSettings = GetDefault<UGameDataSettings>();

	return GDSettings->GetLoadedDataAsset(GDSettings->AbilityInfo);
}

// ---------------------------------------------------------------------------------------------------------------------
UStatusInfo* UGeoAbilitySystemLibrary::GetStatusInfo(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	const UGameDataSettings* GDSettings = GetDefault<UGameDataSettings>();

	return GDSettings->GetLoadedDataAsset(GDSettings->StatusInfo);
}

// ---------------------------------------------------------------------------------------------------------------------
FGameplayEffectContextHandle UGeoAbilitySystemLibrary::ApplyEffectFromEffectData(TArray<FEffectData>& DataArray,
	UGeoAbilitySystemComponent* SourceASC, UGeoAbilitySystemComponent* TargetASC, int32 AbilityLevel, int32 Seed)
{
	FGameplayEffectContextHandle ContextHandle;
	checkf(SourceASC, TEXT("AbilitySystemLibrary::ApplyEffectFromDamageParams: needs a valid Source ASC to apply effect")) if (
		!IsValid(SourceASC) || !IsValid(TargetASC))
	{
		return ContextHandle;
	}

	ContextHandle = SourceASC->MakeEffectContext();
	ContextHandle.AddSourceObject(SourceASC->GetAvatarActor());

	FGeoGameplayEffectContext* GeoEffectContext = static_cast<FGeoGameplayEffectContext*>(ContextHandle.Get());
	checkf(GeoEffectContext,
		TEXT("AbilitySystemLibrary::ApplyEffectFromDamageParams: Failed to create GeoEffectContext"));

	for (FEffectData& EffectData : DataArray)
	{
		EffectData.UpdateContextHandle(GeoEffectContext);
		EffectData.ApplyEffect(ContextHandle, SourceASC, TargetASC, AbilityLevel, Seed);
	}

	return ContextHandle;
}

FGameplayEffectContextHandle UGeoAbilitySystemLibrary::ApplyEffectFromDamageParams(const FDamageEffectParams& params)
{
	FGameplayEffectContextHandle contextHandle;
	UGeoAbilitySystemComponent* pSourceASC = Cast<UGeoAbilitySystemComponent>(params.SourceASC);
	checkf(pSourceASC, TEXT("AbilitySystemLibrary::ApplyEffectFromDamageParams: needs a valid Source ASC to apply effect")) if (
		!IsValid(pSourceASC) || !IsValid(params.TargetASC))
	{
		return contextHandle;
	}
	checkf(params.DamageGameplayEffectClass,
		TEXT("AbilitySystemLibrary::ApplyEffectFromDamageParams: Damage effect class not set"))

		contextHandle = pSourceASC->MakeEffectContext();
	contextHandle.AddSourceObject(pSourceASC->GetAvatarActor());

	FGeoGameplayEffectContext* pGeoEffectContext = static_cast<FGeoGameplayEffectContext*>(contextHandle.Get());
	checkf(pGeoEffectContext,
		TEXT("AbilitySystemLibrary::ApplyEffectFromDamageParams: Failed to create GeoEffectContext"))

		/** Knockbacks **/
		pGeoEffectContext->SetDeathImpulseVector(params.DeathImpulseVector);
	pGeoEffectContext->SetKnockbackVector(params.KnockbackVector);

	/** Radial damage **/
	pGeoEffectContext->SetIsRadialDamage(params.bIsRadialDamage);
	pGeoEffectContext->SetRadialDamageInnerRadius(params.RadialDamageInnerRadius);
	pGeoEffectContext->SetRadialDamageOuterRadius(params.RadialDamageOuterRadius);
	pGeoEffectContext->SetRadialDamageOrigin(params.RadialDamageOrigin);

	FGameplayEffectSpecHandle specHandle =
		params.SourceASC->MakeOutgoingSpec(params.DamageGameplayEffectClass, params.AbilityLevel, contextHandle);

	/** Type of damage **/
	const FGeoGameplayTags& tags = FGeoGameplayTags::Get();
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
	UAbilitySystemComponent* pSourceASC, const FGameplayTag& statusTag, int32 level)
{
	if (!pTargetASC || !pSourceASC)
	{
		return false;
	}

	const UGameDataSettings* GDSettings = GetDefault<UGameDataSettings>();

	const UStatusInfo* pStatusInfos = GDSettings->GetLoadedDataAsset(GDSettings->StatusInfo);
	if (!pStatusInfos)
	{
		return false;
	}

	// INFOS
	FRpgStatusInfo statusInfo;
	if (!pStatusInfos->FillStatusInfoFromTag(statusTag, statusInfo))
	{
		return false;
	}

	if (!IsValid(statusInfo.StatusEffect))
	{
		UE_VLOG(pSourceASC, LogGeoASC, VeryVerbose, TEXT("No valid effect registered for Status %s"),
			*statusInfo.StatusDisplayName);
		return false;
	}

	FGameplayEffectContextHandle contextHandle = pSourceASC->MakeEffectContext();
	contextHandle.AddSourceObject(pSourceASC->GetAvatarActor());

	FGameplayEffectSpecHandle specHandle = pSourceASC->MakeOutgoingSpec(statusInfo.StatusEffect, level, contextHandle);

	pSourceASC->ApplyGameplayEffectSpecToTarget(*specHandle.Data, pTargetASC);

	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
FGameplayTag UGeoAbilitySystemLibrary::GetGameplayTagFromRootTagString(const FString& StringOfTag)
{
	static TMap<FString, FGameplayTag> RootTagNameToTagMap{};
	if (!RootTagNameToTagMap.Find(StringOfTag))
	{
		RootTagNameToTagMap.Add(StringOfTag, FGameplayTag::RequestGameplayTag(FName(StringOfTag)));
	}
	return RootTagNameToTagMap[StringOfTag];
}

// ---------------------------------------------------------------------------------------------------------------------
FGameplayTag UGeoAbilitySystemLibrary::GetAbilityTagFromSpec(const FGameplayAbilitySpec& Spec)
{
	if (!Spec.Ability)
	{
		return FGameplayTag();
	}

	return GetAbilityTagFromAbility(*Spec.Ability.Get());
}

// ---------------------------------------------------------------------------------------------------------------------
FGameplayTag UGeoAbilitySystemLibrary::GetAbilityTagFromAbility(const UGameplayAbility& Ability)
{
	const FGameplayTag tagToMatch = GetGameplayTagFromRootTagString(RootTagNames::AbilityIDTag);
	for (const FGameplayTag& currentTag : Ability.GetAssetTags())
	{
		if (currentTag.MatchesTag(tagToMatch))
		{
			return currentTag;
		}
	}
	return FGameplayTag();
}

// ---------------------------------------------------------------------------------------------------------------------
TArray<AActor*> UGeoAbilitySystemLibrary::GetAllAgentsInTeam(const UObject* WorldContextObject,
	const FGenericTeamId& TeamId)
{
	TArray<AActor*> Result;

	if (!WorldContextObject || !WorldContextObject->GetWorld())
	{
		UE_LOG(LogGeoASC, Warning, TEXT("No World in %s"), *FString(__FUNCTION__));
		return Result;
	}

	for (TActorIterator<AActor> It(WorldContextObject->GetWorld()); It; ++It)
	{
		AActor* Actor = *It;

		IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(Actor);
		if (!TeamAgent)
		{
			continue;
		}

		const FGenericTeamId OtherTeam = TeamAgent->GetGenericTeamId();

		if (OtherTeam == TeamId)
		{
			Result.Add(Actor);
		}
	}

	return Result;
}

// ---------------------------------------------------------------------------------------------------------------------
TArray<AActor*> UGeoAbilitySystemLibrary::GetAllAgentsWithRelationTowardsActor(const UObject* WorldContextObject,
	const AActor* Actor, ETeamAttitude::Type Attitude)
{
	TArray<AActor*> Result;

	if (!WorldContextObject || !WorldContextObject->GetWorld())
	{
		UE_LOG(LogGeoASC, Warning, TEXT("No World in %s"), *FString(__FUNCTION__));
		return Result;
	}
	if (!Actor)
	{
		UE_LOG(LogGeoASC, Warning, TEXT("No Actor in %s"), *FString(__FUNCTION__));
		return Result;
	}

	for (TActorIterator<AActor> It(WorldContextObject->GetWorld()); It; ++It)
	{
		AActor* OtherActor = *It;

		const IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(OtherActor);
		if (!TeamAgent)
		{
			continue;
		}

		if (const APlayerState* PS = Cast<APlayerState>(OtherActor))
		{
			OtherActor = PS->GetPawn();
		}
		else if (const AController* Controller = Cast<AController>(OtherActor))
		{
			OtherActor = Controller->GetPawn();
		}

		if (Attitude == TeamAgent->GetTeamAttitudeTowards(*Actor))
		{
			Result.AddUnique(OtherActor);
		}
	}

	return Result;
}

// ---------------------------------------------------------------------------------------------------------------------
AActor* UGeoAbilitySystemLibrary::GetNearestActorFromList(const AActor* FromActor, const TArray<AActor*>& ActorList)
{
	if (!IsValid(FromActor) || ActorList.Num() == 0)
	{
		return nullptr;
	}
	AActor* NearestActor = nullptr;
	float NearestDistanceSqr = TNumericLimits<float>::Max();
	const FVector FromLocation = FromActor->GetActorLocation();
	for (AActor* CurrentActor : ActorList)
	{
		if (!IsValid(CurrentActor))
		{
			continue;
		}
		const float CurrentDistanceSqr = FVector::DistSquared(FromLocation, CurrentActor->GetActorLocation());
		if (CurrentDistanceSqr < NearestDistanceSqr)
		{
			NearestDistanceSqr = CurrentDistanceSqr;
			NearestActor = CurrentActor;
		}
	}
	return NearestActor;
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoAbilitySystemLibrary::IsBlockedHit(const FGameplayEffectContextHandle& effectContextHandle)
{
	const FGeoGameplayEffectContext* pGeoContext =
		static_cast<const FGeoGameplayEffectContext*>(effectContextHandle.Get());

	return pGeoContext ? pGeoContext->IsBlockedHit() : false;
}

bool UGeoAbilitySystemLibrary::IsCriticalHit(const FGameplayEffectContextHandle& effectContextHandle)
{
	const FGeoGameplayEffectContext* pGeoContext =
		static_cast<const FGeoGameplayEffectContext*>(effectContextHandle.Get());

	return pGeoContext ? pGeoContext->IsCriticalHit() : false;
}

bool UGeoAbilitySystemLibrary::IsSuccessfulDebuff(const FGameplayEffectContextHandle& effectContextHandle)
{
	const FGeoGameplayEffectContext* pGeoContext =
		static_cast<const FGeoGameplayEffectContext*>(effectContextHandle.Get());

	return pGeoContext ? pGeoContext->GetStatusTag().IsValid() : false;
}

float UGeoAbilitySystemLibrary::GetDebuffDamage(const FGameplayEffectContextHandle& effectContextHandle)
{
	const FGeoGameplayEffectContext* pGeoContext =
		static_cast<const FGeoGameplayEffectContext*>(effectContextHandle.Get());

	return pGeoContext ? pGeoContext->GetDebuffDamage() : 0.f;
}

float UGeoAbilitySystemLibrary::GetDebuffDuration(const FGameplayEffectContextHandle& effectContextHandle)
{
	const FGeoGameplayEffectContext* pGeoContext =
		static_cast<const FGeoGameplayEffectContext*>(effectContextHandle.Get());

	return pGeoContext ? pGeoContext->GetDebuffDamage() : 0.f;
}

float UGeoAbilitySystemLibrary::GetDebuffFrequency(const FGameplayEffectContextHandle& effectContextHandle)
{
	const FGeoGameplayEffectContext* pGeoContext =
		static_cast<const FGeoGameplayEffectContext*>(effectContextHandle.Get());

	return pGeoContext ? pGeoContext->GetDebuffFrequency() : 0.f;
}

FVector UGeoAbilitySystemLibrary::GetDeathImpulseVector(const FGameplayEffectContextHandle& effectContextHandle)
{
	const FGeoGameplayEffectContext* pGeoContext =
		static_cast<const FGeoGameplayEffectContext*>(effectContextHandle.Get());

	return pGeoContext ? pGeoContext->GetDeathImpulseVector() : FVector::ZeroVector;
}

FVector UGeoAbilitySystemLibrary::GetKnockbackVector(const FGameplayEffectContextHandle& effectContextHandle)
{
	const FGeoGameplayEffectContext* pGeoContext =
		static_cast<const FGeoGameplayEffectContext*>(effectContextHandle.Get());

	return pGeoContext ? pGeoContext->GetKnockbackVector() : FVector::ZeroVector;
}

bool UGeoAbilitySystemLibrary::GetIsRadialDamage(const FGameplayEffectContextHandle& effectContextHandle)
{
	const FGeoGameplayEffectContext* pGeoContext =
		static_cast<const FGeoGameplayEffectContext*>(effectContextHandle.Get());

	return pGeoContext ? pGeoContext->GetIsRadialDamage() : false;
}

float UGeoAbilitySystemLibrary::GetRadialDamageInnerRadius(const FGameplayEffectContextHandle& effectContextHandle)
{
	const FGeoGameplayEffectContext* pGeoContext =
		static_cast<const FGeoGameplayEffectContext*>(effectContextHandle.Get());

	return pGeoContext ? pGeoContext->GetRadialDamageInnerRadius() : 0.f;
}

float UGeoAbilitySystemLibrary::GetRadialDamageOuterRadius(const FGameplayEffectContextHandle& effectContextHandle)
{
	const FGeoGameplayEffectContext* pGeoContext =
		static_cast<const FGeoGameplayEffectContext*>(effectContextHandle.Get());

	return pGeoContext ? pGeoContext->GetRadialDamageOuterRadius() : 0.f;
}

FVector UGeoAbilitySystemLibrary::GetRadialDamageOrigin(const FGameplayEffectContextHandle& effectContextHandle)
{
	const FGeoGameplayEffectContext* pGeoContext =
		static_cast<const FGeoGameplayEffectContext*>(effectContextHandle.Get());

	return pGeoContext ? pGeoContext->GetRadialDamageOrigin() : FVector::ZeroVector;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySystemLibrary::SetIsBlockedHit(FGameplayEffectContextHandle& effectContextHandle,
	const bool bIsBlockedHit)
{
	FGeoGameplayEffectContext* pGeoContext = static_cast<FGeoGameplayEffectContext*>(effectContextHandle.Get());
	if (pGeoContext)
	{
		pGeoContext->SetIsBlockedHit(bIsBlockedHit);
	}
}

void UGeoAbilitySystemLibrary::SetIsCriticalHit(FGameplayEffectContextHandle& effectContextHandle,
	const bool bIsCriticalHit)
{
	FGeoGameplayEffectContext* pGeoContext = static_cast<FGeoGameplayEffectContext*>(effectContextHandle.Get());
	if (pGeoContext)
	{
		pGeoContext->SetIsCriticalHit(bIsCriticalHit);
	}
}

void UGeoAbilitySystemLibrary::SetStatusTag(FGameplayEffectContextHandle& effectContextHandle,
	const FGameplayTag statusTag)
{
	FGeoGameplayEffectContext* pGeoContext = static_cast<FGeoGameplayEffectContext*>(effectContextHandle.Get());
	if (pGeoContext)
	{
		pGeoContext->SetStatusTag(statusTag);
	}
}

void UGeoAbilitySystemLibrary::SetDebuffDamage(FGameplayEffectContextHandle& effectContextHandle,
	const float debuffDamage)
{
	FGeoGameplayEffectContext* pGeoContext = static_cast<FGeoGameplayEffectContext*>(effectContextHandle.Get());
	if (pGeoContext)
	{
		pGeoContext->SetDebuffDamage(debuffDamage);
	}
}

void UGeoAbilitySystemLibrary::SetDebuffFrequency(FGameplayEffectContextHandle& effectContextHandle,
	const float debuffFrequency)
{
	FGeoGameplayEffectContext* pGeoContext = static_cast<FGeoGameplayEffectContext*>(effectContextHandle.Get());
	if (pGeoContext)
	{
		pGeoContext->SetDebuffFrequency(debuffFrequency);
	}
}

void UGeoAbilitySystemLibrary::SetDebuffDuration(FGameplayEffectContextHandle& effectContextHandle,
	const float debuffDuration)
{
	FGeoGameplayEffectContext* pGeoContext = static_cast<FGeoGameplayEffectContext*>(effectContextHandle.Get());
	if (pGeoContext)
	{
		pGeoContext->SetDebuffDuration(debuffDuration);
	}
}

void UGeoAbilitySystemLibrary::SetDeathImpulseVector(FGameplayEffectContextHandle& effectContextHandle,
	const FVector& inVector)
{
	FGeoGameplayEffectContext* pGeoContext = static_cast<FGeoGameplayEffectContext*>(effectContextHandle.Get());
	if (pGeoContext)
	{
		pGeoContext->SetDeathImpulseVector(inVector);
	}
}

void UGeoAbilitySystemLibrary::SetKnockbackVector(FGameplayEffectContextHandle& effectContextHandle,
	const FVector& inVector)
{
	FGeoGameplayEffectContext* pGeoContext = static_cast<FGeoGameplayEffectContext*>(effectContextHandle.Get());
	if (pGeoContext)
	{
		pGeoContext->SetKnockbackVector(inVector);
	}
}

void UGeoAbilitySystemLibrary::SetIsRadialDamage(FGameplayEffectContextHandle& effectContextHandle,
	const bool bIsRadialDamage)
{
	FGeoGameplayEffectContext* pGeoContext = static_cast<FGeoGameplayEffectContext*>(effectContextHandle.Get());
	if (pGeoContext)
	{
		pGeoContext->SetIsRadialDamage(bIsRadialDamage);
	}
}

void UGeoAbilitySystemLibrary::SetRadialDamageInnerRadius(FGameplayEffectContextHandle& effectContextHandle,
	const float radialDamageInnerRadius)
{
	FGeoGameplayEffectContext* pGeoContext = static_cast<FGeoGameplayEffectContext*>(effectContextHandle.Get());
	if (pGeoContext)
	{
		pGeoContext->SetRadialDamageInnerRadius(radialDamageInnerRadius);
	}
}

void UGeoAbilitySystemLibrary::SetRadialDamageOuterRadius(FGameplayEffectContextHandle& effectContextHandle,
	const float radialDamageOuterRadius)
{
	FGeoGameplayEffectContext* pGeoContext = static_cast<FGeoGameplayEffectContext*>(effectContextHandle.Get());
	if (pGeoContext)
	{
		pGeoContext->SetRadialDamageOuterRadius(radialDamageOuterRadius);
	}
}

void UGeoAbilitySystemLibrary::SetRadialDamageOrigin(FGameplayEffectContextHandle& effectContextHandle,
	const FVector& inVector)
{
	FGeoGameplayEffectContext* pGeoContext = static_cast<FGeoGameplayEffectContext*>(effectContextHandle.Get());
	if (pGeoContext)
	{
		pGeoContext->SetRadialDamageOrigin(inVector);
	}
}

TArray<FEffectData> UGeoAbilitySystemLibrary::GetEffectDataArray(UEffectDataAsset* EffectDataAsset)
{
	if (!ensureMsgf(IsValid(EffectDataAsset), TEXT("EffectDataAsset is not valid !")))
	{
		return {};
	}

	TArray<FEffectData> EffectDataArray;

	EffectDataArray.Reserve(EffectDataAsset->EffectDataInstances.Num());

	for (const FInstancedStruct& Effect : EffectDataAsset->EffectDataInstances)
	{
		checkf(Effect.IsValid() && Effect.GetScriptStruct()->IsChildOf(FEffectData::StaticStruct()),
			TEXT("Effects must be from type FEffectData"));

		EffectDataArray.Add(Effect.Get<FEffectData>());
	}

	return EffectDataArray;
}