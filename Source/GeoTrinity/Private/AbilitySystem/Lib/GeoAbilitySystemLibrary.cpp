// Copyright 2024 GeoTrinity. All Rights Reserved.

// ReSharper disable CppUE4CodingStandardNamingViolationWarning
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystem/Data/GeoCueParam.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "AbilitySystem/Types/GeoAscTypes.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Actor/Deployable/GeoDeployableBase.h"
#include "Actor/GeoInteractableActor.h"
#include "Actor/Projectile/ExternalProjectileParams.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "Characters/Component/GeoGameFeelComponent.h"
#include "Characters/GeoCharacter.h"
#include "EngineUtils.h"
#include "GameplayEffectTypes.h"
#include "GeoTrinity/GeoTrinity.h"
#include "InstancedStruct.h"
#include "Kismet/GameplayStatics.h"
#include "Settings/GameDataSettings.h"
#include "System/GeoActorPoolingSubsystem.h"
#include "System/GeoPoolableInterface.h"
#include "Tool/UGeoGameplayLibrary.h"

// ---------------------------------------------------------------------------------------------------------------------
UAbilityInfo* UGeoAbilitySystemLibrary::GetAbilityInfo()
{
	UGameDataSettings const* GDSettings = GetDefault<UGameDataSettings>();
	return GDSettings->GetLoadedDataAsset(GDSettings->AbilityInfo);
}

// ---------------------------------------------------------------------------------------------------------------------
FGameplayEffectContextHandle UGeoAbilitySystemLibrary::MakeGeoEffectContext(UAbilitySystemComponent* SourceASC,
																			UAbilitySystemComponent* TargetASC,
																			FGeoGameplayEffectContext*& OutGeoContext)
{
	FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
	FillEffectContext(SourceASC, TargetASC, ContextHandle);

	OutGeoContext = static_cast<FGeoGameplayEffectContext*>(ContextHandle.Get());
	checkf(OutGeoContext, TEXT("%hs: failed to create the Geo effect context"), __FUNCTION__);
	return ContextHandle;
}

FActiveGameplayEffectHandle UGeoAbilitySystemLibrary::ApplySingleEffectData(TInstancedStruct<FEffectData> const& Data,
																			UAbilitySystemComponent* SourceASC,
																			UAbilitySystemComponent* TargetASC,
																			int32 AbilityLevel, int32 Seed,
																			FGameplayTag AbilityTag)
{
	FEffectData const* EffectData = Data.GetPtr<FEffectData>();
	checkf(EffectData, TEXT("%hs: invalid EffectData"), __FUNCTION__);
	return ApplySingleEffectData(*EffectData, SourceASC, TargetASC, AbilityLevel, Seed, AbilityTag);
}

FActiveGameplayEffectHandle UGeoAbilitySystemLibrary::ApplySingleEffectData(FEffectData const& EffectData,
																			UAbilitySystemComponent* SourceASC,
																			UAbilitySystemComponent* TargetASC,
																			int32 AbilityLevel, int32 Seed,
																			FGameplayTag AbilityTag)
{
	if (!ensureMsgf(IsValid(SourceASC) && IsValid(TargetASC),
					TEXT("%hs: needs a valid Source and Target ASC to apply an effect"), __FUNCTION__)
		|| !EffectData.AppliesAtLevel(AbilityLevel))
	{
		return FActiveGameplayEffectHandle();
	}

	FGeoGameplayEffectContext* GeoEffectContext = nullptr;
	FGameplayEffectContextHandle ContextHandle = MakeGeoEffectContext(SourceASC, TargetASC, GeoEffectContext);

	EffectData.UpdateContextHandle(GeoEffectContext, AbilityLevel, AbilityTag);
	FActiveGameplayEffectHandle const EffectHandle =
		EffectData.ApplyEffect(ContextHandle, SourceASC, TargetASC, AbilityLevel, Seed);
	EffectData.ExecuteOneShotCue(ContextHandle, TargetASC, AbilityLevel, AbilityTag);
	return EffectHandle;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySystemLibrary::NotifyAbilityHit(FAbilityPayload const& Payload, AActor* HitActor)
{
	if (!Payload.HitNotified.IsValid() || *Payload.HitNotified)
	{
		return;
	}

	UGeoAbilitySystemComponent* InstigatorASC = GetGeoAscFromActor(Payload.Owner);
	if (!IsValid(InstigatorASC))
	{
		return;
	}

	*Payload.HitNotified = true;
	InstigatorASC->OnAbilityHit.Broadcast(Payload.AbilityTag, Payload.Instigator, HitActor);
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoAbilitySystemLibrary::ShouldSuppressGameplayCue(FGeoGameplayEffectContext const& GeoContext,
														 AActor* TargetAvatar, bool const bIsHeal)
{
	if (GeoContext.IsSuppressGameplayCue())
	{
		return true;
	}
	if (!GeoContext.IsLimitGameplayCue() || !IsValid(TargetAvatar))
	{
		return false;
	}

	UGeoGameFeelComponent* GameFeelComponent = TargetAvatar->FindComponentByClass<UGeoGameFeelComponent>();
	if (!ensureMsgf(GameFeelComponent, TEXT("%hs: bLimitGameplayCue set but target %s has no GeoGameFeelComponent"),
					__FUNCTION__, *TargetAvatar->GetName()))
	{
		return false;
	}
	return !GameFeelComponent->IsCueAvailable(bIsHeal);
}

void UGeoAbilitySystemLibrary::FillEffectContext(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC,
												 FGameplayEffectContextHandle ContextHandle)
{
	AActor* SourceAvatarActor = IsValid(SourceASC) ? SourceASC->GetAvatarActor() : nullptr;
	if (IsValid(SourceAvatarActor))
	{
		ContextHandle.AddSourceObject(SourceAvatarActor);
	}


	if (AActor* TargetAvatar = TargetASC->GetAvatarActor())
	{
		FHitResult HitResult;
		HitResult.ImpactPoint = TargetAvatar->GetActorLocation();
		HitResult.ImpactNormal = IsValid(SourceAvatarActor)
			? (TargetAvatar->GetActorLocation() - SourceAvatarActor->GetActorLocation()).GetSafeNormal2D()
			: FVector::Zero();
		ContextHandle.AddHitResult(HitResult);
	}
}
// ---------------------------------------------------------------------------------------------------------------------
TArray<FActiveGameplayEffectHandle> UGeoAbilitySystemLibrary::ApplyEffectFromEffectData(
	TArray<TInstancedStruct<FEffectData>> const& DataArray, UAbilitySystemComponent* SourceASC,
	UAbilitySystemComponent* TargetASC, int32 AbilityLevel, int32 Seed, FGameplayTag AbilityTag)
{
	TArray<FActiveGameplayEffectHandle> SpecHandles;
	if (!ensureMsgf(IsValid(SourceASC) && IsValid(TargetASC),
					TEXT("%hs: needs a valid Source and Target ASC to apply an effect"), __FUNCTION__))
	{
		return SpecHandles;
	}

	FGeoGameplayEffectContext* GeoEffectContext = nullptr;
	FGameplayEffectContextHandle ContextHandle = MakeGeoEffectContext(SourceASC, TargetASC, GeoEffectContext);

	TArray<FEffectData const*> ApplicableEffects;
	ApplicableEffects.Reserve(DataArray.Num());
	for (auto const& EffectDataInstance : DataArray)
	{
		FEffectData const* EffectData = EffectDataInstance.GetPtr<FEffectData>();
		checkf(EffectData, TEXT("%hs: invalid EffectData"), __FUNCTION__);
		if (EffectData->AppliesAtLevel(AbilityLevel))
		{
			ApplicableEffects.Add(EffectData);
		}
	}

	for (FEffectData const* EffectData : ApplicableEffects)
	{
		EffectData->UpdateContextHandle(GeoEffectContext, AbilityLevel, AbilityTag);
	}

	for (FEffectData const* EffectData : ApplicableEffects)
	{
		SpecHandles.Add(EffectData->ApplyEffect(ContextHandle, SourceASC, TargetASC, AbilityLevel, Seed));
		EffectData->ExecuteOneShotCue(ContextHandle, TargetASC, AbilityLevel, AbilityTag);
	}

	return SpecHandles;
}

// ---------------------------------------------------------------------------------------------------------------------
FGameplayTag UGeoAbilitySystemLibrary::GetFirstAssetTagUnderRoot(UGameplayAbility const& Ability, FGameplayTag Root)
{
	for (FGameplayTag const& AssetTag : Ability.GetAssetTags())
	{
		if (AssetTag.MatchesTag(Root))
		{
			return AssetTag;
		}
	}
	return FGameplayTag();
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
	static FGameplayTag const SpellRoot = FGameplayTag::RequestGameplayTag(FName(RootTagNames::AbilitySpellTag));
	return GetFirstAssetTagUnderRoot(Ability, SpellRoot);
}

// ---------------------------------------------------------------------------------------------------------------------
TArray<AActor*> UGeoAbilitySystemLibrary::GetInteractableActors(UObject const* WorldContextObject,
																FGenericTeamId const SourceTeam, int32 AttitudeBitmask,
																bool bMustBeDamageable, FVector2D const Location,
																float MaxDistance,
																TFunctionRef<bool(AActor*)> const& ExtraFilter,
																ETargetOverlapMode OverlapMode)
{
	TArray<AActor*> Result;

	if (!WorldContextObject || !WorldContextObject->GetWorld())
	{
		UE_LOG(LogGeoASC, Warning, TEXT("No World in %s"), *FString(__FUNCTION__));
		return Result;
	}

	bool const bHasDistanceCheck = MaxDistance > 0.f;
	bool const bIncludeTargetRadius = ShouldIncludeTargetRadius(OverlapMode, SourceTeam);

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

		float const OtherActorRadius = bIncludeTargetRadius ? OtherActor->GetSimpleCollisionRadius() : 0.f;
		float const DistanceThreshold = MaxDistance + OtherActorRadius;
		if (bHasDistanceCheck
			&& FVector2D::DistSquared(Location, FVector2D(OtherActor->GetActorLocation()))
				> DistanceThreshold * DistanceThreshold)
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
																float MaxDistance, ETargetOverlapMode OverlapMode)
{
	return GetInteractableActors(
		WorldContextObject, SourceTeam, AttitudeBitmask, bMustBeDamageable, Location, MaxDistance,
		[](AActor*)
		{
			return true;
		},
		OverlapMode);
}

TArray<AActor*> UGeoAbilitySystemLibrary::GetInteractableActorsInLine(
	UObject const* WorldContextObject, FGenericTeamId const SourceTeam, int32 AttitudeBitmask, bool bMustBeDamageable,
	FVector2D const Origin, FVector2D const ForwardVector, float const MaxRange, float const LineHalfWidth,
	ETargetOverlapMode OverlapMode)
{
	bool const bIncludeTargetRadius = ShouldIncludeTargetRadius(OverlapMode, SourceTeam);
	return GetInteractableActors(
		WorldContextObject, SourceTeam, AttitudeBitmask, bMustBeDamageable, Origin, MaxRange,
		[&](AActor const* Target)
		{
			FVector2D const ToTarget = FVector2D(Target->GetActorLocation()) - Origin;
			float const AlongBeam = FVector2D::DotProduct(ToTarget, ForwardVector);
			if (AlongBeam < 0.f)
			{
				return false;
			}
			float const PerpDistSqr = (ToTarget - ForwardVector * AlongBeam).SizeSquared();
			float const TargetRadius = bIncludeTargetRadius ? Target->GetSimpleCollisionRadius() : 0.f;
			float const HitRadius = TargetRadius + LineHalfWidth;
			return PerpDistSqr <= HitRadius * HitRadius;
		},
		OverlapMode);
}

bool UGeoAbilitySystemLibrary::ShouldIncludeTargetRadius(ETargetOverlapMode OverlapMode,
														 FGenericTeamId const SourceTeam)
{
	switch (OverlapMode)
	{
	case ETargetOverlapMode::IncludeRadius:
		return true;
	case ETargetOverlapMode::CenterOnly:
		return false;
	case ETargetOverlapMode::Automatic:
	default:
		return SourceTeam.GetId() != static_cast<uint8>(ETeam::Enemy);
	}
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
void UGeoAbilitySystemLibrary::ExecuteGeoCue(UAbilitySystemComponent* ASC, FGeoCueParam const& Cue,
											 FGameplayCueParameters const& CueParams, bool const bLocalOnly)
{
	if (!Cue.IsValid()
		|| !ensureMsgf(IsValid(ASC), TEXT("%hs: null ASC for cue %s"), __FUNCTION__, *Cue.CueTag.ToString()))
	{
		return;
	}

	auto const Execute = [ASC, &CueParams, bLocalOnly](FGameplayTag const& Tag)
	{
		if (bLocalOnly)
		{
			ASC->InvokeGameplayCueEvent(Tag, EGameplayCueEvent::Executed, CueParams);
		}
		else
		{
			ASC->ExecuteGameplayCue(Tag, CueParams);
		}
	};

	if (Cue.CueTag.IsValid())
	{
		Execute(Cue.CueTag);
	}

	FGameplayTag const SoundCueTag = GetDefault<UGameDataSettings>()->GenericGameplayCueSoundTag;
	if (Cue.SoundTag.IsValid()
		&& ensureMsgf(SoundCueTag.IsValid(),
					  TEXT("%hs: sound %s needs UGameDataSettings::GenericGameplayCueSoundTag set"), __FUNCTION__,
					  *Cue.SoundTag.ToString()))
	{
		Execute(SoundCueTag);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
FGameplayTag UGeoAbilitySystemLibrary::GetStatusTag(FGameplayEffectContextHandle const& EffectContextHandle)
{
	FGeoGameplayEffectContext const* GeoContext =
		static_cast<FGeoGameplayEffectContext const*>(EffectContextHandle.Get());

	return GeoContext ? GeoContext->GetStatusTag() : FGameplayTag{};
}

void UGeoAbilitySystemLibrary::SetStatusTag(FGameplayEffectContextHandle& EffectContextHandle, FGameplayTag StatusTag)
{
	if (FGeoGameplayEffectContext* GeoContext = static_cast<FGeoGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		GeoContext->SetStatusTag(StatusTag);
	}
}

UGeoAbilitySystemComponent* UGeoAbilitySystemLibrary::GetGeoAscFromActor(AActor* Actor)
{
	return Cast<UGeoAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor));
}

AActor* UGeoAbilitySystemLibrary::GetAvatarFromActor(AActor* Actor)
{
	UGeoAbilitySystemComponent const* const ASC = GetGeoAscFromActor(Actor);
	return ASC ? ASC->GetAvatarActor() : nullptr;
}

UGeoGameplayAbility const* UGeoAbilitySystemLibrary::GetAbilityCDO(FGameplayTag const AbilityTag)
{
	return GetAbilityCDO<UGeoGameplayAbility>(AbilityTag);
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
	if (!ensureMsgf(IsValid(Deployable), TEXT("%hs: failed to spawn %s"), __FUNCTION__,
					*DeployableActorClass->GetName()))
	{
		return nullptr;
	}

	FDeployableData Data;
	FillDeployableData(Data, Payload, EffectDataArray, Params);
	Deployable->InitInteractable(&Data);

	if (!ensureMsgf(Deployable->IsActive(), TEXT("%hs: %s went inactive during init"), __FUNCTION__,
					*DeployableActorClass->GetName()))
	{
		return nullptr;
	}
	Deployable->FinishSpawning(SpawnTransform);

	return Deployable;
}

AGeoDeployableBase*
UGeoAbilitySystemLibrary::StartSpawnDeployable(TSubclassOf<AGeoDeployableBase> const DeployableActorClass,
											   AActor* Owner, APawn* Instigator, FTransform const& SpawnTransform)
{
	if (!ensureMsgf(IsValid(DeployableActorClass) && IsValid(Owner),
					TEXT("%hs: needs a DeployableActorClass and a valid Owner"), __FUNCTION__))
	{
		return nullptr;
	}

	if (!IsValid(Instigator))
	{
		UE_LOG(LogTemp, Error, TEXT("%hs: no valid pawn to spawn the deployable from"), __FUNCTION__);
		return nullptr;
	}

	AGeoDeployableBase* Deployable = Owner->GetWorld()->SpawnActorDeferred<AGeoDeployableBase>(
		DeployableActorClass, SpawnTransform, Owner, Instigator, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (!IsValid(Deployable))
	{
		UE_LOG(LogTemp, Error, TEXT("%hs: failed to spawn %s"), __FUNCTION__, *DeployableActorClass->GetName());
		return nullptr;
	}

	return Deployable;
}

// ---------------------------------------------------------------------------------------------------------------------

void UGeoAbilitySystemLibrary::FillDeployableData(FDeployableData& Data, FAbilityPayload const& Payload,
												  TArray<TInstancedStruct<FEffectData>> const& EffectDataArray,
												  FDeployableDataParams const& Params)
{
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
}

AGeoProjectile*
UGeoAbilitySystemLibrary::FullySpawnProjectile(UWorld* const World, FExternalProjectileParams const& Params,
											   FTransform const& SpawnTransform, FAbilityPayload const& Payload,
											   TArray<TInstancedStruct<FEffectData>> const& EffectDataArray,
											   float const SpawnServerTime, FPredictionKey PredictionKey)
{
	AGeoProjectile* Projectile =
		StartSpawnProjectile(World, Params, SpawnTransform, Payload, EffectDataArray, PredictionKey);
	if (!Projectile)
	{
		return nullptr;
	}
	FinishSpawnProjectile(World, Projectile, SpawnTransform, SpawnServerTime, PredictionKey);
	return Projectile;
}

int32 UGeoAbilitySystemLibrary::SpawnProjectileSpread(UWorld* const World, FExternalProjectileParams const& Params,
													  EProjectileTarget const Target, FVector const& Origin,
													  float const Yaw, float const SpawnServerTime,
													  FAbilityPayload const& Payload,
													  TArray<TInstancedStruct<FEffectData>> const& EffectDataArray,
													  FPredictionKey PredictionKey)
{
	if (!ensureMsgf(Params.ProjectileClass, TEXT("SpawnProjectileSpread: no ProjectileClass set!")))
	{
		return 0;
	}

	int32 SpawnedCount = 0;
	for (FVector const& Direction : GetTargetDirections(World, Target, Yaw, Origin))
	{
		FTransform const SpawnTransform{Direction.Rotation().Quaternion(), Origin};
		if (ensureMsgf(FullySpawnProjectile(World, Params, SpawnTransform, Payload, EffectDataArray, SpawnServerTime,
											PredictionKey),
					   TEXT("SpawnProjectileSpread: failed to spawn projectile!")))
		{
			++SpawnedCount;
		}
	}

	return SpawnedCount;
}

AGeoProjectile*
UGeoAbilitySystemLibrary::StartSpawnProjectile(UWorld* const World, FExternalProjectileParams const& Params,
											   FTransform const& SpawnTransform, FAbilityPayload const& Payload,
											   TArray<TInstancedStruct<FEffectData>> const& EffectDataArray,
											   FPredictionKey PredictionKey)
{
	if (!World || !Params.ProjectileClass)
	{
		UE_LOG(LogTemp, Error, TEXT("%hs: invalid World or ProjectileClass"), __FUNCTION__);
		return nullptr;
	}

	AGeoProjectile* Projectile = nullptr;
	if (Params.ProjectileClass->ImplementsInterface(UGeoPoolableInterface::StaticClass()))
	{
		Projectile = UGeoActorPoolingSubsystem::Get(World)->RequestActor(
			Params.ProjectileClass, SpawnTransform, Payload.Owner, Cast<APawn>(Payload.Instigator), false);
	}
	else
	{
		Projectile = World->SpawnActorDeferred<AGeoProjectile>(Params.ProjectileClass, SpawnTransform, Payload.Owner,
															   Cast<APawn>(Payload.Instigator),
															   ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	}

	if (!Projectile)
	{
		UE_LOG(LogTemp, Error, TEXT("%hs: no projectile came back for %s"), __FUNCTION__,
			   *Params.ProjectileClass->GetName());
		return nullptr;
	}

	Projectile->Payload = Payload;
	Projectile->EffectDataArray = EffectDataArray;
	Projectile->PredictionKeyId = PredictionKey.Current;
	Projectile->ApplyProjectileParams(Params);

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
	}

	if (GeoLib::IsServer(World))
	{
		float const TimeDelta = FMath::Clamp(GeoLib::GetServerTime(World) - SpawnServerTime, 0.f,
											 GetDefault<UGameDataSettings>()->MaxLatencyCompensation);
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

	IGenericTeamAgentInterface const* const OwnerTeamInterface = Cast<IGenericTeamAgentInterface const>(Owner);
	if (!ensureMsgf(OwnerTeamInterface, TEXT("%hs: %s has no team interface"), __FUNCTION__, *Owner->GetName()))
	{
		return false;
	}

	return IsAttitudeIntBitflag(static_cast<ETeamAttitudeBitflag>(OverlapAttitudeBitMask),
								OwnerTeamInterface->GetTeamAttitudeTowards(*OtherActor));
}

int UGeoAbilitySystemLibrary::GetAndCheckSection(UAnimMontage const* AnimMontage, FName const Section)
{
	int const SectionIndex = AnimMontage->GetSectionIndex(Section);
	ensureMsgf(SectionIndex != INDEX_NONE, TEXT("%hs: section %s not found in AnimMontage %s"), __FUNCTION__,
			   *Section.ToString(), *AnimMontage->GetName());
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
	IGenericTeamAgentInterface const* const TeamInterface = Cast<IGenericTeamAgentInterface const>(Actor);
	return TeamInterface ? TeamInterface->GetGenericTeamId() : FGenericTeamId::NoTeam;
}

bool UGeoAbilitySystemLibrary::HasEffectInArray(TArray<TInstancedStruct<FEffectData>> const& EffectDataArray,
												UScriptStruct const* const EffectDataType)
{
	return EffectDataArray.ContainsByPredicate(
		[EffectDataType](TInstancedStruct<FEffectData> const& EffectData)
		{
			return EffectData.GetScriptStruct() && EffectData.GetScriptStruct()->IsChildOf(EffectDataType);
		});
}

bool UGeoAbilitySystemLibrary::IsBuffed(UGeoAbilitySystemComponent const& ASC, FGameplayAttribute const& Attribute)
{
	return ASC.HasAttributeSetForAttribute(Attribute)
		&& ASC.GetNumericAttribute(Attribute) > ASC.GetNumericAttributeBase(Attribute);
}

float UGeoAbilitySystemLibrary::GetEffectBoostBonus(UAbilitySystemComponent const& ASC,
													FGameplayEffectSpec const& Spec)
{
	float Bonus = 0.f;
	for (int32 ModifierIndex = 0; ModifierIndex < Spec.Modifiers.Num(); ++ModifierIndex)
	{
		FGameplayModifierInfo const& Modifier = Spec.Def->Modifiers[ModifierIndex];
		if (!ASC.HasAttributeSetForAttribute(Modifier.Attribute))
		{
			continue;
		}

		// Only a ratio stat has a percentage to show: a multiplier sits at 1 unboosted, a share like DamageReduction
		// at 0. Anything else (health, ammo) is a count, and its modifiers are not a boost.
		float const BaseValue = ASC.GetNumericAttributeBase(Modifier.Attribute);
		if (!FMath::IsNearlyZero(BaseValue) && !FMath::IsNearlyEqual(BaseValue, 1.f))
		{
			continue;
		}

		// Stacking and the bias a multiplying op carries (1.5 means +50%, 0.5 on an adding op means the same) are the
		// aggregator's own rules — take them from it rather than restating them here.
		float const StackedMagnitude = GameplayEffectUtilities::ComputeStackedModifierMagnitude(
			Spec.Modifiers[ModifierIndex].GetEvaluatedMagnitude(), Spec.GetStackCount(), Modifier.ModifierOp);
		Bonus += StackedMagnitude - GameplayEffectUtilities::GetModifierBiasByModifierOp(Modifier.ModifierOp);
	}

	return Bonus;
}
