// Fill out your copyright notice in the Description page of Project Settings.

// ReSharper disable CppUE4CodingStandardNamingViolationWarning
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"

#include "AbilitySystem/Abilities/GeoGameplayAbility.h"
#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystem/Data/StatusInfo.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystem/GeoAscTypes.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerState.h"
#include "GameplayEffectTypes.h"
#include "GeoTrinity/GeoTrinity.h"
#include "InstancedStruct.h"
#include "Settings/GameDataSettings.h"

// ---------------------------------------------------------------------------------------------------------------------
UAbilityInfo* UGeoAbilitySystemLibrary::GetAbilityInfo(UObject const* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	UGameDataSettings const* GDSettings = GetDefault<UGameDataSettings>();

	return GDSettings->GetLoadedDataAsset(GDSettings->AbilityInfo);
}

UAbilityInfo* UGeoAbilitySystemLibrary::GetAbilityInfo()
{
	UGameDataSettings const* GDSettings = GetDefault<UGameDataSettings>();
	return GDSettings->GetLoadedDataAsset(GDSettings->AbilityInfo);
}

// ---------------------------------------------------------------------------------------------------------------------
UStatusInfo* UGeoAbilitySystemLibrary::GetStatusInfo(UObject const* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	UGameDataSettings const* GDSettings = GetDefault<UGameDataSettings>();

	return GDSettings->GetLoadedDataAsset(GDSettings->StatusInfo);
}

// ---------------------------------------------------------------------------------------------------------------------
FGameplayEffectContextHandle UGeoAbilitySystemLibrary::ApplyEffectFromEffectData(
	TArray<TInstancedStruct<FEffectData>> const& DataArray, UGeoAbilitySystemComponent* SourceASC,
	UGeoAbilitySystemComponent* TargetASC, int32 AbilityLevel, int32 Seed)
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

	for (auto EffectDataInstance : DataArray)
	{
		FEffectData const* EffectData = EffectDataInstance.GetPtr<FEffectData>();
		checkf(EffectData, TEXT("AbilitySystemLibrary::ApplyEffectFromDamageParams: Invalid EffectData"));
		EffectData->UpdateContextHandle(GeoEffectContext);
		EffectData->ApplyEffect(ContextHandle, SourceASC, TargetASC, AbilityLevel, Seed);
	}

	return ContextHandle;
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoAbilitySystemLibrary::ApplyStatusToTarget(UAbilitySystemComponent* pTargetASC,
												   UAbilitySystemComponent* pSourceASC, FGameplayTag const& statusTag,
												   int32 level)
{
	if (!pTargetASC || !pSourceASC)
	{
		return false;
	}

	UGameDataSettings const* GDSettings = GetDefault<UGameDataSettings>();

	UStatusInfo const* pStatusInfos = GDSettings->GetLoadedDataAsset(GDSettings->StatusInfo);
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
FGameplayTag UGeoAbilitySystemLibrary::GetGameplayTagFromRootTagString(FString const& StringOfTag)
{
	static TMap<FString, FGameplayTag> RootTagNameToTagMap{};
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
	{
		return FGameplayTag();
	}

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
TArray<AActor*> UGeoAbilitySystemLibrary::GetAllAgentsInTeam(UObject const* WorldContextObject,
															 FGenericTeamId const& TeamId)
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

		FGenericTeamId const OtherTeam = TeamAgent->GetGenericTeamId();

		if (OtherTeam == TeamId)
		{
			Result.Add(Actor);
		}
	}

	return Result;
}

// ---------------------------------------------------------------------------------------------------------------------
TArray<AActor*> UGeoAbilitySystemLibrary::GetAllAgentsWithRelationTowardsActor(UObject const* WorldContextObject,
																			   AActor const* Actor,
																			   ETeamAttitude::Type Attitude)
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

		IGenericTeamAgentInterface const* TeamAgent = Cast<IGenericTeamAgentInterface>(OtherActor);
		if (!TeamAgent)
		{
			continue;
		}

		if (APlayerState const* PS = Cast<APlayerState>(OtherActor))
		{
			OtherActor = PS->GetPawn();
		}
		else if (AController const* Controller = Cast<AController>(OtherActor))
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
AActor* UGeoAbilitySystemLibrary::GetNearestActorFromList(AActor const* FromActor, TArray<AActor*> const& ActorList)
{
	if (!IsValid(FromActor) || ActorList.Num() == 0)
	{
		return nullptr;
	}
	AActor* NearestActor = nullptr;
	float NearestDistanceSqr = TNumericLimits<float>::Max();
	FVector const FromLocation = FromActor->GetActorLocation();
	for (AActor* CurrentActor : ActorList)
	{
		if (!IsValid(CurrentActor))
		{
			continue;
		}
		float const CurrentDistanceSqr = FVector::DistSquared(FromLocation, CurrentActor->GetActorLocation());
		if (CurrentDistanceSqr < NearestDistanceSqr)
		{
			NearestDistanceSqr = CurrentDistanceSqr;
			NearestActor = CurrentActor;
		}
	}
	return NearestActor;
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoAbilitySystemLibrary::IsBlockedHit(FGameplayEffectContextHandle const& effectContextHandle)
{
	FGeoGameplayEffectContext const* pGeoContext =
		static_cast<FGeoGameplayEffectContext const*>(effectContextHandle.Get());

	return pGeoContext ? pGeoContext->IsBlockedHit() : false;
}

bool UGeoAbilitySystemLibrary::IsCriticalHit(FGameplayEffectContextHandle const& effectContextHandle)
{
	FGeoGameplayEffectContext const* pGeoContext =
		static_cast<FGeoGameplayEffectContext const*>(effectContextHandle.Get());

	return pGeoContext ? pGeoContext->IsCriticalHit() : false;
}

bool UGeoAbilitySystemLibrary::IsSuccessfulDebuff(FGameplayEffectContextHandle const& effectContextHandle)
{
	FGeoGameplayEffectContext const* pGeoContext =
		static_cast<FGeoGameplayEffectContext const*>(effectContextHandle.Get());

	return pGeoContext ? pGeoContext->GetStatusTag().IsValid() : false;
}

float UGeoAbilitySystemLibrary::GetDebuffDamage(FGameplayEffectContextHandle const& effectContextHandle)
{
	FGeoGameplayEffectContext const* pGeoContext =
		static_cast<FGeoGameplayEffectContext const*>(effectContextHandle.Get());

	return pGeoContext ? pGeoContext->GetDebuffDamage() : 0.f;
}

float UGeoAbilitySystemLibrary::GetDebuffDuration(FGameplayEffectContextHandle const& effectContextHandle)
{
	FGeoGameplayEffectContext const* pGeoContext =
		static_cast<FGeoGameplayEffectContext const*>(effectContextHandle.Get());

	return pGeoContext ? pGeoContext->GetDebuffDamage() : 0.f;
}

float UGeoAbilitySystemLibrary::GetDebuffFrequency(FGameplayEffectContextHandle const& effectContextHandle)
{
	FGeoGameplayEffectContext const* pGeoContext =
		static_cast<FGeoGameplayEffectContext const*>(effectContextHandle.Get());

	return pGeoContext ? pGeoContext->GetDebuffFrequency() : 0.f;
}

FVector UGeoAbilitySystemLibrary::GetDeathImpulseVector(FGameplayEffectContextHandle const& effectContextHandle)
{
	FGeoGameplayEffectContext const* pGeoContext =
		static_cast<FGeoGameplayEffectContext const*>(effectContextHandle.Get());

	return pGeoContext ? pGeoContext->GetDeathImpulseVector() : FVector::ZeroVector;
}

FVector UGeoAbilitySystemLibrary::GetKnockbackVector(FGameplayEffectContextHandle const& effectContextHandle)
{
	FGeoGameplayEffectContext const* pGeoContext =
		static_cast<FGeoGameplayEffectContext const*>(effectContextHandle.Get());

	return pGeoContext ? pGeoContext->GetKnockbackVector() : FVector::ZeroVector;
}

bool UGeoAbilitySystemLibrary::GetIsRadialDamage(FGameplayEffectContextHandle const& effectContextHandle)
{
	FGeoGameplayEffectContext const* pGeoContext =
		static_cast<FGeoGameplayEffectContext const*>(effectContextHandle.Get());

	return pGeoContext ? pGeoContext->GetIsRadialDamage() : false;
}

float UGeoAbilitySystemLibrary::GetRadialDamageInnerRadius(FGameplayEffectContextHandle const& effectContextHandle)
{
	FGeoGameplayEffectContext const* pGeoContext =
		static_cast<FGeoGameplayEffectContext const*>(effectContextHandle.Get());

	return pGeoContext ? pGeoContext->GetRadialDamageInnerRadius() : 0.f;
}

float UGeoAbilitySystemLibrary::GetRadialDamageOuterRadius(FGameplayEffectContextHandle const& effectContextHandle)
{
	FGeoGameplayEffectContext const* pGeoContext =
		static_cast<FGeoGameplayEffectContext const*>(effectContextHandle.Get());

	return pGeoContext ? pGeoContext->GetRadialDamageOuterRadius() : 0.f;
}

FVector UGeoAbilitySystemLibrary::GetRadialDamageOrigin(FGameplayEffectContextHandle const& effectContextHandle)
{
	FGeoGameplayEffectContext const* pGeoContext =
		static_cast<FGeoGameplayEffectContext const*>(effectContextHandle.Get());

	return pGeoContext ? pGeoContext->GetRadialDamageOrigin() : FVector::ZeroVector;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySystemLibrary::SetIsBlockedHit(FGameplayEffectContextHandle& effectContextHandle,
											   bool const bIsBlockedHit)
{
	FGeoGameplayEffectContext* pGeoContext = static_cast<FGeoGameplayEffectContext*>(effectContextHandle.Get());
	if (pGeoContext)
	{
		pGeoContext->SetIsBlockedHit(bIsBlockedHit);
	}
}

void UGeoAbilitySystemLibrary::SetIsCriticalHit(FGameplayEffectContextHandle& effectContextHandle,
												bool const bIsCriticalHit)
{
	FGeoGameplayEffectContext* pGeoContext = static_cast<FGeoGameplayEffectContext*>(effectContextHandle.Get());
	if (pGeoContext)
	{
		pGeoContext->SetIsCriticalHit(bIsCriticalHit);
	}
}

void UGeoAbilitySystemLibrary::SetStatusTag(FGameplayEffectContextHandle& effectContextHandle,
											FGameplayTag const statusTag)
{
	FGeoGameplayEffectContext* pGeoContext = static_cast<FGeoGameplayEffectContext*>(effectContextHandle.Get());
	if (pGeoContext)
	{
		pGeoContext->SetStatusTag(statusTag);
	}
}

void UGeoAbilitySystemLibrary::SetDebuffDamage(FGameplayEffectContextHandle& effectContextHandle,
											   float const debuffDamage)
{
	FGeoGameplayEffectContext* pGeoContext = static_cast<FGeoGameplayEffectContext*>(effectContextHandle.Get());
	if (pGeoContext)
	{
		pGeoContext->SetDebuffDamage(debuffDamage);
	}
}

void UGeoAbilitySystemLibrary::SetDebuffFrequency(FGameplayEffectContextHandle& effectContextHandle,
												  float const debuffFrequency)
{
	FGeoGameplayEffectContext* pGeoContext = static_cast<FGeoGameplayEffectContext*>(effectContextHandle.Get());
	if (pGeoContext)
	{
		pGeoContext->SetDebuffFrequency(debuffFrequency);
	}
}

void UGeoAbilitySystemLibrary::SetDebuffDuration(FGameplayEffectContextHandle& effectContextHandle,
												 float const debuffDuration)
{
	FGeoGameplayEffectContext* pGeoContext = static_cast<FGeoGameplayEffectContext*>(effectContextHandle.Get());
	if (pGeoContext)
	{
		pGeoContext->SetDebuffDuration(debuffDuration);
	}
}

void UGeoAbilitySystemLibrary::SetDeathImpulseVector(FGameplayEffectContextHandle& effectContextHandle,
													 FVector const& inVector)
{
	FGeoGameplayEffectContext* pGeoContext = static_cast<FGeoGameplayEffectContext*>(effectContextHandle.Get());
	if (pGeoContext)
	{
		pGeoContext->SetDeathImpulseVector(inVector);
	}
}

void UGeoAbilitySystemLibrary::SetKnockbackVector(FGameplayEffectContextHandle& effectContextHandle,
												  FVector const& inVector)
{
	FGeoGameplayEffectContext* pGeoContext = static_cast<FGeoGameplayEffectContext*>(effectContextHandle.Get());
	if (pGeoContext)
	{
		pGeoContext->SetKnockbackVector(inVector);
	}
}

void UGeoAbilitySystemLibrary::SetIsRadialDamage(FGameplayEffectContextHandle& effectContextHandle,
												 bool const bIsRadialDamage)
{
	FGeoGameplayEffectContext* pGeoContext = static_cast<FGeoGameplayEffectContext*>(effectContextHandle.Get());
	if (pGeoContext)
	{
		pGeoContext->SetIsRadialDamage(bIsRadialDamage);
	}
}

void UGeoAbilitySystemLibrary::SetRadialDamageInnerRadius(FGameplayEffectContextHandle& effectContextHandle,
														  float const radialDamageInnerRadius)
{
	FGeoGameplayEffectContext* pGeoContext = static_cast<FGeoGameplayEffectContext*>(effectContextHandle.Get());
	if (pGeoContext)
	{
		pGeoContext->SetRadialDamageInnerRadius(radialDamageInnerRadius);
	}
}

void UGeoAbilitySystemLibrary::SetRadialDamageOuterRadius(FGameplayEffectContextHandle& effectContextHandle,
														  float const radialDamageOuterRadius)
{
	FGeoGameplayEffectContext* pGeoContext = static_cast<FGeoGameplayEffectContext*>(effectContextHandle.Get());
	if (pGeoContext)
	{
		pGeoContext->SetRadialDamageOuterRadius(radialDamageOuterRadius);
	}
}

void UGeoAbilitySystemLibrary::SetRadialDamageOrigin(FGameplayEffectContextHandle& effectContextHandle,
													 FVector const& inVector)
{
	FGeoGameplayEffectContext* pGeoContext = static_cast<FGeoGameplayEffectContext*>(effectContextHandle.Get());
	if (pGeoContext)
	{
		pGeoContext->SetRadialDamageOrigin(inVector);
	}
}

UGeoAbilitySystemComponent* UGeoAbilitySystemLibrary::GetGeoAscFromActor(AActor* Actor)
{
	return Cast<UGeoAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor));
}

TArray<TInstancedStruct<FEffectData>>
UGeoAbilitySystemLibrary::GetEffectDataArray(UEffectDataAsset const* EffectDataAsset)
{
	if (!ensureMsgf(IsValid(EffectDataAsset), TEXT("EffectDataAsset is not valid !")))
	{
		return {};
	}

	return EffectDataAsset->EffectDataInstances;
}

TArray<TInstancedStruct<FEffectData>> UGeoAbilitySystemLibrary::GetEffectDataArray(FGameplayTag AbilityTag)
{
	for (auto AbilityInfo : GetAbilityInfo()->AbilityInfos)
	{
		if (AbilityInfo.AbilityTag == AbilityTag)
		{
			UGameplayAbility const* AbilityCDO = AbilityInfo.AbilityClass.GetDefaultObject();
			if (IsValid(AbilityCDO) && AbilityCDO->IsA(UGeoGameplayAbility::StaticClass()))
			{
				return CastChecked<UGeoGameplayAbility>(AbilityCDO)->GetEffectDataArray();
			}
		}
	}

	ensureMsgf(true, TEXT("No EffectData found for AbilityTag %s"), *AbilityTag.ToString());
	return {};
}
