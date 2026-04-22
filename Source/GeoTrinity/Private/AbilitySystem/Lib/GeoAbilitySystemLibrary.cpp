// Copyright 2024 GeoTrinity. All Rights Reserved.

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
#include "Actor/GeoInteractableActor.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "Characters/GeoCharacter.h"
#include "EngineUtils.h"
#include "GameplayEffectTypes.h"
#include "GeoTrinity/GeoTrinity.h"
#include "InstancedStruct.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraTypes.h"
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
																			int32 AbilityLevel, int32 Seed)
{
	FEffectData const* EffectData = Data.GetPtr<FEffectData>();
	checkf(EffectData, TEXT("AbilitySystemLibrary::ApplyEffectFromDamageParams: Invalid EffectData"));
	return ApplySingleEffectData(*EffectData, SourceASC, TargetASC, AbilityLevel, Seed);
}

FActiveGameplayEffectHandle UGeoAbilitySystemLibrary::ApplySingleEffectData(FEffectData const& EffectData,
																			UAbilitySystemComponent* SourceASC,
																			UAbilitySystemComponent* TargetASC,
																			int32 AbilityLevel, int32 Seed)
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
	ContextHandle.AddSourceObject(SourceASC->GetAvatarActor());
	FGeoGameplayEffectContext* GeoEffectContext = static_cast<FGeoGameplayEffectContext*>(ContextHandle.Get());

	EffectData.UpdateContextHandle(GeoEffectContext, AbilityLevel);
	return EffectData.ApplyEffect(ContextHandle, SourceASC, TargetASC, AbilityLevel, Seed);
}

// ---------------------------------------------------------------------------------------------------------------------
TArray<FActiveGameplayEffectHandle>
UGeoAbilitySystemLibrary::ApplyEffectFromEffectData(TArray<TInstancedStruct<FEffectData>> const& DataArray,
													UAbilitySystemComponent* SourceASC,
													UAbilitySystemComponent* TargetASC, int32 AbilityLevel, int32 Seed)
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
	ContextHandle.AddSourceObject(SourceASC->GetAvatarActor());

	FGeoGameplayEffectContext* GeoEffectContext = static_cast<FGeoGameplayEffectContext*>(ContextHandle.Get());
	checkf(GeoEffectContext,
		   TEXT("AbilitySystemLibrary::ApplyEffectFromDamageParams: Failed to create GeoEffectContext"));

	for (auto const& EffectDataInstance : DataArray)
	{
		FEffectData const* EffectData = EffectDataInstance.GetPtr<FEffectData>();
		checkf(EffectData, TEXT("AbilitySystemLibrary::ApplyEffectFromDamageParams: Invalid EffectData"));
		EffectData->UpdateContextHandle(GeoEffectContext, AbilityLevel);
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

	for (TActorIterator<AGeoCharacter> It(WorldContextObject->GetWorld()); It; ++It)
	{
		AActor* OtherActor = *It;

		IGenericTeamAgentInterface const* OtherActorTeamInterface = Cast<IGenericTeamAgentInterface>(OtherActor);
		checkf(OtherActorTeamInterface, TEXT(" AGeoCharacter is a IGenericTeamAgentInterface, this should never fail"));

		if (Attitude == OtherActorTeamInterface->GetTeamAttitudeTowards(*Actor))
		{
			Result.AddUnique(OtherActor);
		}
	}

	for (TActorIterator<AGeoInteractableActor> It(WorldContextObject->GetWorld()); It; ++It)
	{
		AActor* OtherActor = *It;

		IGenericTeamAgentInterface const* OtherActorTeamInterface = Cast<IGenericTeamAgentInterface>(OtherActor);
		checkf(OtherActorTeamInterface,
			   TEXT(" AGeoInteractableActor is a IGenericTeamAgentInterface, this should never fail"));

		if (Attitude == OtherActorTeamInterface->GetTeamAttitudeTowards(*Actor))
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
	for (auto AbilityInfo : GetAbilityInfo()->GetAllAbilityInfos())
	{
		if (AbilityInfo->AbilityTag == AbilityTag)
		{
			UGameplayAbility const* AbilityCDO = AbilityInfo->AbilityClass.GetDefaultObject();
			if (IsValid(AbilityCDO) && AbilityCDO->IsA(UGeoGameplayAbility::StaticClass()))
			{
				return CastChecked<UGeoGameplayAbility>(AbilityCDO)->GetEffectDataArray();
			}
		}
	}

	ensureMsgf(false, TEXT("No Ability found for AbilityTag %s"), *AbilityTag.ToString());
	return {};
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
	bool const bIsPoolable = ProjectileClass->ImplementsInterface(UGeoPoolableInterface::StaticClass());
	if (bIsPoolable)
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
		UGameplayStatics::FinishSpawningActor(Projectile, SpawnTransform);
		Projectile->InitProjectileLife();

		// Register prediction key delegates as fast-path cleanup for when the ability
		// is confirmed AFTER the projectile spawns (no FireRate delay).
		// For the FireRate-delayed case, BeginPlay proximity matching handles it instead.
		// When LocalOnlyProjectiles is on, keep the fake alive (server projectile won't replicate to owner).
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

bool UGeoAbilitySystemLibrary::IsAttitudeIntBitflag(ETeamAttitudeBitflag AttitudeBitflag, ETeamAttitude::Type Attitude)
{
	return (static_cast<uint8>(AttitudeBitflag) & static_cast<uint8>(GetAttitudeBitflag(Attitude))) != 0x00;
}

bool UGeoAbilitySystemLibrary::IsTeamAttitudeAligned(AActor const* Owner, AActor const* OtherActor,
													 int32 OverlapAttitudeBitMask)
{
	IAbilitySystemInterface const* OwnerASCInterface = Cast<IAbilitySystemInterface>(Owner);
	if (!OwnerASCInterface)
	{
		UE_LOG(LogGeoTrinity, Error,
			   TEXT("The Owner does not implement IAbilitySystemInterface. It should never happen"));
		return false;
	}

	AActor const* SourceAvatarActor = OwnerASCInterface->GetAbilitySystemComponent()->GetAvatarActor();
	if (!IsValid(SourceAvatarActor))
	{
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

bool UGeoAbilitySystemLibrary::GetTeamInterface(AActor const* Actor, IGenericTeamAgentInterface const*& OutInterface)
{
	OutInterface = Cast<IGenericTeamAgentInterface const>(Actor);
	return OutInterface != nullptr;
}
