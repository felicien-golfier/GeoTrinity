// Copyright 2024 GeoTrinity. All Rights Reserved.

// ReSharper disable CppUE4CodingStandardNamingViolationWarning
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystem/Data/StatusInfo.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "AbilitySystem/Types/GeoAscTypes.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Actor/Deployable/GeoDeployableBase.h"
#include "Actor/GeoInteractableActor.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "Characters/GeoCharacter.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerState.h"
#include "GameplayEffectTypes.h"
#include "GeoTrinity/GeoTrinity.h"
#include "InstancedStruct.h"
#include "Kismet/GameplayStatics.h"
#include "Settings/GameDataSettings.h"
#include "System/GeoActorPoolingSubsystem.h"
#include "System/GeoPoolableInterface.h"
#include "Tool/UGeoGameplayLibrary.h"

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
FActiveGameplayEffectHandle UGeoAbilitySystemLibrary::ApplySingleEffectData(TInstancedStruct<FEffectData> const& Data,
																			UAbilitySystemComponent* SourceASC,
																			UAbilitySystemComponent* TargetASC,
																			int32 AbilityLevel, int32 Seed,
																			FGameplayTag AbilityTag)
{
	FEffectData const* EffectData = Data.GetPtr<FEffectData>();
	checkf(EffectData, TEXT("AbilitySystemLibrary::ApplyEffectFromDamageParams: Invalid EffectData"));
	return ApplySingleEffectData(*EffectData, SourceASC, TargetASC, AbilityLevel, Seed, AbilityTag);
}

FActiveGameplayEffectHandle UGeoAbilitySystemLibrary::ApplySingleEffectData(FEffectData const& EffectData,
																			UAbilitySystemComponent* SourceASC,
																			UAbilitySystemComponent* TargetASC,
																			int32 AbilityLevel, int32 Seed,
																			FGameplayTag AbilityTag)
{
	if (!IsValid(SourceASC) || !IsValid(TargetASC))
	{
		ensureMsgf(
			IsValid(SourceASC) && IsValid(TargetASC),
			TEXT(
				"AbilitySystemLibrary::ApplyEffectFromDamageParams: needs a valid Source and Target ASC to apply effect"));
		return FActiveGameplayEffectHandle();
	}

	FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
	FillEffectContext(SourceASC, TargetASC, ContextHandle);

	FGeoGameplayEffectContext* GeoEffectContext = static_cast<FGeoGameplayEffectContext*>(ContextHandle.Get());

	EffectData.UpdateContextHandle(GeoEffectContext, AbilityLevel, AbilityTag);
	return EffectData.ApplyEffect(ContextHandle, SourceASC, TargetASC, AbilityLevel, Seed);
}

void UGeoAbilitySystemLibrary::FillEffectContext(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC,
												 FGameplayEffectContextHandle ContextHandle)
{
	ContextHandle.AddSourceObject(SourceASC->GetAvatarActor());

	if (AActor* TargetAvatar = TargetASC->GetAvatarActor())
	{
		FHitResult HitResult;
		HitResult.ImpactPoint = TargetAvatar->GetActorLocation();
		HitResult.ImpactNormal =
			(TargetAvatar->GetActorLocation() - SourceASC->GetAvatarActor()->GetActorLocation()).GetSafeNormal2D();
		ContextHandle.AddHitResult(HitResult);
	}
}
// ---------------------------------------------------------------------------------------------------------------------
TArray<FActiveGameplayEffectHandle>
UGeoAbilitySystemLibrary::ApplyEffectFromEffectData(TArray<TInstancedStruct<FEffectData>> const& DataArray,
													UAbilitySystemComponent* SourceASC,
													UAbilitySystemComponent* TargetASC, int32 AbilityLevel, int32 Seed,
													FGameplayTag AbilityTag)
{
	TArray<FActiveGameplayEffectHandle> SpecHandles;
	if (!IsValid(SourceASC) || !IsValid(TargetASC))
	{
		ensureMsgf(
			IsValid(SourceASC) && IsValid(TargetASC),
			TEXT(
				"AbilitySystemLibrary::ApplyEffectFromDamageParams: needs a valid Source and Target ASC to apply effect"));
		return SpecHandles;
	}

	FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
	FillEffectContext(SourceASC, TargetASC, ContextHandle);

	FGeoGameplayEffectContext* GeoEffectContext = static_cast<FGeoGameplayEffectContext*>(ContextHandle.Get());
	checkf(GeoEffectContext,
		   TEXT("AbilitySystemLibrary::ApplyEffectFromDamageParams: Failed to create GeoEffectContext"));

	for (auto const& EffectDataInstance : DataArray)
	{
		FEffectData const* EffectData = EffectDataInstance.GetPtr<FEffectData>();
		checkf(EffectData, TEXT("AbilitySystemLibrary::ApplyEffectFromDamageParams: Invalid EffectData"));
		EffectData->UpdateContextHandle(GeoEffectContext, AbilityLevel, AbilityTag);
	}

	for (auto const& EffectDataInstance : DataArray)
	{
		FEffectData const* EffectData = EffectDataInstance.GetPtr<FEffectData>();
		SpecHandles.Add(EffectData->ApplyEffect(ContextHandle, SourceASC, TargetASC, AbilityLevel, Seed));
	}

	return SpecHandles;
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoAbilitySystemLibrary::ApplyStatusToTarget(UAbilitySystemComponent* pTargetASC,
												   UAbilitySystemComponent* pSourceASC, FGameplayTag const& statusTag,
												   int32 level, FGameplayEffectSpecHandle& OutSpecHandle)
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

	OutSpecHandle = pSourceASC->MakeOutgoingSpec(statusInfo.StatusEffect, level, contextHandle);

	pSourceASC->ApplyGameplayEffectSpecToTarget(*OutSpecHandle.Data, pTargetASC);

	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
/**
 * Resolves a tag string to a FGameplayTag and caches the result.
 * RequestGameplayTag involves a hash-map lookup inside the tags manager on every call;
 * the local static cache avoids that cost for frequently queried root tags.
 */
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
	FGameplayTag const tagToMatch = GetGameplayTagFromRootTagString(RootTagNames::AbilitySpellTag);
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
TArray<AActor*> UGeoAbilitySystemLibrary::GetInteractableActors(UObject const* WorldContextObject,
																FGenericTeamId const SourceTeam, int32 AttitudeBitmask,
																bool bMustBeDamageable, FVector2D const Location,
																float MaxDistance,
																TFunctionRef<bool(AActor*)> const& ExtraFilter)
{
	TArray<AActor*> Result;

	if (!WorldContextObject || !WorldContextObject->GetWorld())
	{
		UE_LOG(LogGeoASC, Warning, TEXT("No World in %s"), *FString(__FUNCTION__));
		return Result;
	}

	bool const bHasDistanceCheck = MaxDistance > 0.f;
	float const MaxDistanceSqr = MaxDistance * MaxDistance;

	auto TryAddActor = [&](AActor* OtherActor, TCHAR const* ClassName)
	{
		if (!IsValid(OtherActor))
		{
			return;
		}

		IGenericTeamAgentInterface const* TeamInterface = Cast<IGenericTeamAgentInterface>(OtherActor);
		checkf(TeamInterface, TEXT("%s is a IGenericTeamAgentInterface, this should never fail"), ClassName);

		if (bMustBeDamageable && !OtherActor->CanBeDamaged())
		{
			return;
		}

		if (SourceTeam != FGenericTeamId::NoTeam
			&& !IsAttitudeIntBitflag(static_cast<ETeamAttitudeBitflag>(AttitudeBitmask),
									 FGenericTeamId::GetAttitude(SourceTeam, TeamInterface->GetGenericTeamId())))
		{
			return;
		}

		float const OtherActorRadius = OtherActor->GetSimpleCollisionRadius();
		if (bHasDistanceCheck
			&& FVector2D::DistSquared(Location, FVector2D(OtherActor->GetActorLocation()))
				> MaxDistanceSqr + OtherActorRadius * OtherActorRadius)
		{
			return;
		}

		if (!ExtraFilter(OtherActor))
		{
			return;
		}

		Result.Add(OtherActor);
	};

	for (TActorIterator<AGeoCharacter> It(WorldContextObject->GetWorld()); It; ++It)
	{
		TryAddActor(*It, TEXT("AGeoCharacter"));
	}

	for (TActorIterator<AGeoInteractableActor> It(WorldContextObject->GetWorld()); It; ++It)
	{
		TryAddActor(*It, TEXT("AGeoInteractableActor"));
	}

	return Result;
}

TArray<AActor*> UGeoAbilitySystemLibrary::GetInteractableActors(UObject const* WorldContextObject,
																FGenericTeamId const SourceTeam, int32 AttitudeBitmask,
																bool bMustBeDamageable, FVector2D const Location,
																float MaxDistance)
{
	return GetInteractableActors(WorldContextObject, SourceTeam, AttitudeBitmask, bMustBeDamageable, Location,
								 MaxDistance,
								 [](AActor*)
								 {
									 return true;
								 });
}

TArray<AActor*> UGeoAbilitySystemLibrary::GetInteractableActorsInLine(
	UObject const* WorldContextObject, FGenericTeamId const SourceTeam, int32 AttitudeBitmask, bool bMustBeDamageable,
	FVector2D const Origin, FVector2D const ForwardVector, float const MaxRange, float const LineHalfWidth)
{
	return GetInteractableActors(WorldContextObject, SourceTeam, AttitudeBitmask, bMustBeDamageable, Origin, MaxRange,
								 [&](AActor const* Target)
								 {
									 FVector2D const ToTarget = FVector2D(Target->GetActorLocation()) - Origin;
									 float const AlongBeam = FVector2D::DotProduct(ToTarget, ForwardVector);
									 if (AlongBeam < 0.f)
									 {
										 return false;
									 }
									 float const PerpDistSqr = (ToTarget - ForwardVector * AlongBeam).SizeSquared();
									 float const HitRadius = Target->GetSimpleCollisionRadius() + LineHalfWidth;
									 return PerpDistSqr <= HitRadius * HitRadius;
								 });
}
TArray<AActor*> UGeoAbilitySystemLibrary::GetInteractableActors(UObject const* WorldContextObject,
																FGenericTeamId const SourceTeam, int32 AttitudeBitmask,
																bool bMustBeDamageable)
{
	return GetInteractableActors(WorldContextObject, SourceTeam, AttitudeBitmask, bMustBeDamageable,
								 FVector2D::ZeroVector, 0.0f);
}
TArray<AActor*> UGeoAbilitySystemLibrary::GetInteractableActors(UObject const* WorldContextObject,
																bool bMustBeDamageable, FVector2D Location,
																float MaxDistance)
{
	return GetInteractableActors(WorldContextObject, FGenericTeamId(), 0, bMustBeDamageable, Location, MaxDistance);
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

	return pGeoContext ? pGeoContext->GetDebuffDuration() : 0.f;
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

UGeoGameplayAbility const* UGeoAbilitySystemLibrary::GetAbilityCDO(FGameplayTag const AbilityTag)
{
	return GetAbilityCDO<UGeoGameplayAbility>(AbilityTag);
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

TArray<TInstancedStruct<FEffectData>> UGeoAbilitySystemLibrary::GetEffectDataArray(FGameplayTag const AbilityTag)
{
	if (UGeoGameplayAbility const* AbilityCDO = GetAbilityCDO(AbilityTag))
	{
		return AbilityCDO->GetEffectDataArray();
	}

	return {};
}

AGeoDeployableBase*
UGeoAbilitySystemLibrary::FullySpawnDeployable(TSubclassOf<AGeoDeployableBase> const DeployableActorClass,
											   FAbilityPayload const& Payload,
											   TArray<TInstancedStruct<FEffectData>> const& EffectDataArray,
											   FDeployableDataParams const& Params, FTransform const& SpawnTransform)
{
	AGeoDeployableBase* Deployable =
		StartSpawnDeployable(DeployableActorClass, Payload.Owner, Cast<APawn>(Payload.Instigator), SpawnTransform);
	if (!ensureMsgf(IsValid(Deployable), TEXT("Failed to spawn deployable! %s"), *DeployableActorClass->GetName()))
	{
		return nullptr;
	}
	InitDeployable(Deployable, Payload, EffectDataArray, Params);
	FinishSpawnDeployable(Deployable, SpawnTransform);

	return Deployable;
}

AGeoDeployableBase*
UGeoAbilitySystemLibrary::StartSpawnDeployable(TSubclassOf<AGeoDeployableBase> const DeployableActorClass,
											   AActor* Owner, APawn* Instigator, FTransform const& SpawnTransform)
{
	if (!IsValid(DeployableActorClass))
	{
		ensureMsgf(DeployableActorClass, TEXT("SpawnDeployableActor: No DeployableActorClass set!"));
		return nullptr;
	}

	if (!IsValid(Owner))
	{
		ensureMsgf(Owner, TEXT("SpawnDeployableActor : No valid Owner "));
		return nullptr;
	}

	if (!IsValid(Instigator))
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnDeployableActor: No valid pawn to spawn deployable!"));
		return nullptr;
	}

	AGeoDeployableBase* Deployable = Owner->GetWorld()->SpawnActorDeferred<AGeoDeployableBase>(
		DeployableActorClass, SpawnTransform, Owner, Instigator, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (!IsValid(Deployable))
	{
		UE_LOG(LogTemp, Error, TEXT("DeployableSpawnerProjectile: Failed to spawn deployable!"));
		return nullptr;
	}

	return Deployable;
}

void UGeoAbilitySystemLibrary::FinishSpawnDeployable(AGeoDeployableBase* const Deployable,
													 FTransform const& SpawnTransform)
{
	Deployable->FinishSpawning(SpawnTransform);
}

// ---------------------------------------------------------------------------------------------------------------------

void UGeoAbilitySystemLibrary::InitDeployable(AGeoDeployableBase* Deployable, FAbilityPayload const& Payload,
											  TArray<TInstancedStruct<FEffectData>> const& EffectDataArray,
											  FDeployableDataParams const& Params)
{
	FDeployableData Data;
	Data.Owner = Payload.Owner;
	Data.Instigator = Payload.Instigator;
	Data.Level = Payload.AbilityLevel;
	Data.Seed = Payload.Seed;
	Data.Params = Params;
	Data.EffectDataArray = EffectDataArray;
	Data.AbilityTag = Payload.AbilityTag;
	if (IGenericTeamAgentInterface const* TeamInterface = Cast<IGenericTeamAgentInterface>(Payload.Owner))
	{
		Data.TeamID = TeamInterface->GetGenericTeamId();
	}
	Deployable->InitInteractable(&Data);
}

AGeoProjectile*
UGeoAbilitySystemLibrary::FullySpawnProjectile(UWorld* const World, TSubclassOf<AGeoProjectile> const ProjectileClass,
											   FTransform const& SpawnTransform, FAbilityPayload const& Payload,
											   TArray<TInstancedStruct<FEffectData>> const& EffectDataArray,
											   float const SpawnServerTime, FPredictionKey PredictionKey)
{

	AGeoProjectile* Projectile =
		StartSpawnProjectile(World, ProjectileClass, SpawnTransform, Payload, EffectDataArray, PredictionKey);
	if (!Projectile)
	{
		return nullptr;
	}
	FinishSpawnProjectile(World, Projectile, SpawnTransform, SpawnServerTime, PredictionKey);
	return Projectile;
}

AGeoProjectile*
UGeoAbilitySystemLibrary::StartSpawnProjectile(UWorld* const World, TSubclassOf<AGeoProjectile> const ProjectileClass,
											   FTransform const& SpawnTransform, FAbilityPayload const& Payload,
											   TArray<TInstancedStruct<FEffectData>> const& EffectDataArray,
											   FPredictionKey PredictionKey)
{

	if (!World || !ProjectileClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[UGeoAbilitySystemLibrary::SpawnProjectile] Invalid World or ProjectileClass"));
		return nullptr;
	}

	AGeoProjectile* Projectile;
	if (ProjectileClass->ImplementsInterface(UGeoPoolableInterface::StaticClass()))
	{
		Projectile = UGeoActorPoolingSubsystem::Get(World)->RequestActor(ProjectileClass, SpawnTransform, Payload.Owner,
																		 Cast<APawn>(Payload.Instigator), false);
		if (!Projectile)
		{
			UE_LOG(LogTemp, Error,
				   TEXT("[UGeoAbilitySystemLibrary::SpawnProjectile] RequestActor returned nullptr for %s"),
				   *ProjectileClass->GetName());
			return nullptr;
		}
	}
	else
	{
		Projectile = World->SpawnActorDeferred<AGeoProjectile>(ProjectileClass, SpawnTransform, Payload.Owner,
															   Cast<APawn>(Payload.Instigator),
															   ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

		if (!Projectile)
		{
			UE_LOG(LogTemp, Error,
				   TEXT("[UGeoAbilitySystemLibrary::SpawnProjectile] SpawnActor returned nullptr for %s"),
				   *ProjectileClass->GetName());
			return nullptr;
		}
	}

	Projectile->Payload = Payload;
	Projectile->EffectDataArray = EffectDataArray;
	Projectile->PredictionKeyId = PredictionKey.Current;

	return Projectile;
}

void UGeoAbilitySystemLibrary::FinishSpawnProjectile(UWorld const* World, AGeoProjectile* Projectile,
													 FTransform const& SpawnTransform, float const SpawnServerTime,
													 FPredictionKey PredictionKey)
{
	if (Projectile->GetClass()->ImplementsInterface(UGeoPoolableInterface::StaticClass()))
	{
		Cast<IGeoPoolableInterface>(Projectile)->Init();
	}
	else
	{
		Projectile->InitProjectileLife();
		UGameplayStatics::FinishSpawningActor(Projectile, SpawnTransform);

		// Cache the CVar pointer once rather than re-querying the console manager on every projectile spawn.
		static IConsoleVariable* const LocalOverrideCVar =
			IConsoleManager::Get().FindConsoleVariable(TEXT("Geo.ReplaceLocalProjectiles"));
		ensureMsgf(LocalOverrideCVar,
				   TEXT("LocalOverrideCVar is invalid, please ensure ReplaceLocalProjectiles still exist"));
		if (!GeoLib::IsServer(World) && PredictionKey.IsLocalClientKey() && LocalOverrideCVar->GetBool())
		{
			TWeakObjectPtr<AGeoProjectile> WeakProjectile(Projectile);
			auto DestroyFake = [WeakProjectile]()
			{
				if (WeakProjectile.IsValid())
				{
					WeakProjectile->Destroy();
				}
			};
			PredictionKey.NewCaughtUpDelegate().BindLambda(DestroyFake);
			PredictionKey.NewRejectedDelegate().BindLambda(DestroyFake);
		}
	}

	if (GeoLib::IsServer(World))
	{
		float const TimeDelta = GeoLib::GetServerTime(World) - SpawnServerTime;
		if (TimeDelta > 0.f)
		{
			Projectile->AdvanceProjectile(TimeDelta);
		}
	}
}

TArray<FVector> UGeoAbilitySystemLibrary::GetTargetDirections(UWorld const* World, EProjectileTarget const Target,
															  float const Yaw, FVector const& Origin)
{
	switch (Target)
	{
	case EProjectileTarget::Forward:
		{
			return {FRotator(0.f, Yaw, 0.f).Vector()};
		}

	case EProjectileTarget::AllPlayers:
		{
			TArray<FVector> Directions;
			for (auto PlayerControllerIt = World->GetPlayerControllerIterator(); PlayerControllerIt;
				 ++PlayerControllerIt)
			{
				if (APlayerController const* PlayerController = PlayerControllerIt->Get();
					PlayerController && PlayerController->GetPawn())
				{
					Directions.Add((PlayerController->GetPawn()->GetActorLocation() - Origin).GetSafeNormal());
				}
			}
			return Directions;
		}

	default:
		{
			return {};
		}
	}
}

ETeamAttitudeBitflag UGeoAbilitySystemLibrary::GetAttitudeBitflag(ETeamAttitude::Type Attitude)
{
	switch (Attitude)
	{
	case ETeamAttitude::Neutral:
		return ETeamAttitudeBitflag::Neutral;
	case ETeamAttitude::Hostile:
		return ETeamAttitudeBitflag::Hostile;
	case ETeamAttitude::Friendly:
		return ETeamAttitudeBitflag::Friendly;
	default:
		ensureMsgf(false, TEXT("Invalid team attitude"));
		return ETeamAttitudeBitflag::Neutral;
	}
}

/** Returns true when the attitude bitmask includes the given single attitude value. */
bool UGeoAbilitySystemLibrary::IsAttitudeIntBitflag(ETeamAttitudeBitflag AttitudeBitflag, ETeamAttitude::Type Attitude)
{
	return (static_cast<uint8>(AttitudeBitflag) & static_cast<uint8>(GetAttitudeBitflag(Attitude))) != 0x00;
}

bool UGeoAbilitySystemLibrary::IsTeamAttitudeAligned(AActor const* Owner, AActor const* OtherActor,
													 uint8 OverlapAttitudeBitMask)
{
	if (!IsValid(Owner) || !IsValid(OtherActor))
	{
		UE_LOG(LogGeoTrinity, Warning, TEXT("The Owner or the Other Actor is not valid"));
		return false;
	}

	IGenericTeamAgentInterface const* OwnerTeamInterface = nullptr;
	if (!GetTeamInterface(Owner, OwnerTeamInterface))
	{
		ensureMsgf(false, TEXT("Projectile owner has no team interface"));
		return false;
	}

	return IsAttitudeIntBitflag(static_cast<ETeamAttitudeBitflag>(OverlapAttitudeBitMask),
								OwnerTeamInterface->GetTeamAttitudeTowards(*OtherActor));
}

int UGeoAbilitySystemLibrary::GetAndCheckSection(UAnimMontage const* AnimMontage, FName const Section)
{
	int const SectionIndex = AnimMontage->GetSectionIndex(Section);
	if (SectionIndex == INDEX_NONE)
	{
		ensureMsgf(false, TEXT("Section %s not found in AnimMontage %s"), *Section.ToString(), *AnimMontage->GetName());
		UE_LOG(LogPattern, Error, TEXT("Section %s not found in AnimMontage %s"), *Section.ToString(),
			   *AnimMontage->GetName());
	}
	return SectionIndex;
}

UAnimInstance* UGeoAbilitySystemLibrary::GetAnimInstance(FAbilityPayload const& Payload)
{
	ACharacter* InstigatorCharacter = Cast<ACharacter>(Payload.Instigator);
	if (!IsValid(InstigatorCharacter))
	{
		UE_LOG(LogPattern, Error, TEXT("We support only animation montage for character in pattern for now !"));
		return nullptr;
	}

	UAnimInstance* AnimInstance =
		InstigatorCharacter->GetMesh() ? InstigatorCharacter->GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		UE_LOG(LogPattern, Error, TEXT("Please set an anim instance (With the Default Slot filled in anim graph;)"));
		return nullptr;
	}

	return AnimInstance;
}

FGenericTeamId UGeoAbilitySystemLibrary::GetTeamId(AActor const* Actor)
{
	IGenericTeamAgentInterface const* TeamInterface = nullptr;
	if (GetTeamInterface(Actor, TeamInterface))
	{
		return TeamInterface->GetGenericTeamId();
	}
	return FGenericTeamId::NoTeam;
}

bool UGeoAbilitySystemLibrary::GetTeamInterface(AActor const* Actor, IGenericTeamAgentInterface const*& OutInterface)
{
	OutInterface = Cast<IGenericTeamAgentInterface const>(Actor);
	return OutInterface != nullptr;
}
