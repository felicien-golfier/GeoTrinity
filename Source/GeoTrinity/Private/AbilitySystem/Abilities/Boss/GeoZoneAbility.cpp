// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Boss/GeoZoneAbility.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystemComponent.h"
#include "Actor/Arena/GeoArena.h"
#include "Actor/Deployable/GeoDeployableBase.h"
#include "Actor/Deployable/Zones/GeoEffectZone.h"
#include "Settings/GameDataSettings.h"
#include "Tool/UGeoGameplayLibrary.h"

UGeoZoneAbility::UGeoZoneAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoZoneAbility::ActivateAbility(FGameplayAbilitySpecHandle const Handle,
									  FGameplayAbilityActorInfo const* ActorInfo,
									  FGameplayAbilityActivationInfo const ActivationInfo,
									  FGameplayEventData const* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// A zero fire delay leaves nothing to telegraph: Super already fired and ended the ability by now.
	if (IsActive())
	{
		ExecuteZoneCue(TelegraphCue, GetZoneLocation(), GetFireDelay());
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoZoneAbility::Fire(FGeoAbilityTargetData const& AbilityTargetData)
{
	Super::Fire(AbilityTargetData);

	FVector const ZoneLocation = GetZoneLocation();
	if (ZoneParams.LifeDrainMaxDuration > 0.f)
	{
		GeoASLib::FullySpawnDeployable(GetZoneClass(), StoredPayload, GetEffectDataArray(), ZoneParams,
									   FTransform(ZoneLocation));
	}
	else
	{
		Burst(ZoneLocation);
	}

	EndAbility();
}

// ---------------------------------------------------------------------------------------------------------------------
FVector2D UGeoZoneAbility::GetFireOrigin2D(AActor* Instigator, UGeoAbilitySystemComponent* SourceASC,
										   int const Seed) const
{
	if (!TargetPointTag.IsValid())
	{
		return Super::GetFireOrigin2D(Instigator, SourceASC, Seed);
	}

	AGeoArena const* const Arena = AGeoArena::GetArenaOfBoss(Instigator);
	if (!ensureMsgf(Arena, TEXT("%hs: %s was not spawned by an arena, so it has no points to land on"), __FUNCTION__,
					*GetNameSafe(Instigator)))
	{
		return Super::GetFireOrigin2D(Instigator, SourceASC, Seed);
	}

	TArray<AActor*> const TargetPoints = GeoLib::GetTargetPoints(Instigator, TargetPointTag, Arena->ArenaTag);
	if (!ensureMsgf(!TargetPoints.IsEmpty(), TEXT("%hs: no AGeoTargetPoint tagged %s in arena %s"), __FUNCTION__,
					*TargetPointTag.ToString(), *Arena->ArenaTag.ToString()))
	{
		return Super::GetFireOrigin2D(Instigator, SourceASC, Seed);
	}

	return FVector2D(TargetPoints[0]->GetActorLocation());
}

// ---------------------------------------------------------------------------------------------------------------------
TSubclassOf<AGeoDeployableBase> UGeoZoneAbility::GetZoneClass() const
{
	if (ZoneClass)
	{
		return ZoneClass;
	}

	TSubclassOf<AGeoDeployableBase> const DefaultZoneClass =
		GetDefault<UGameDataSettings>()->DefaultZoneClass.LoadSynchronous();
	ensureMsgf(DefaultZoneClass, TEXT("%hs: no DefaultZoneClass in Game Data Settings and %s names no ZoneClass"),
			   __FUNCTION__, *GetName());
	return DefaultZoneClass;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoZoneAbility::Burst(FVector const& ZoneLocation) const
{
	UGeoAbilitySystemComponent* SourceASC = GetGeoAbilitySystemComponentFromActorInfo();
	TArray<TInstancedStruct<FEffectData>> const EffectDataArray = GetEffectDataArray();
	for (AActor* Target : GeoASLib::GetInteractableActors(this, GeoASLib::GetTeamId(StoredPayload.Owner), BurstAttitude,
														  true, FVector2D(ZoneLocation), ZoneParams.Size))
	{
		if (UGeoAbilitySystemComponent* TargetASC = GeoASLib::GetGeoAscFromActor(Target))
		{
			GeoASLib::ApplyEffectFromEffectData(EffectDataArray, SourceASC, TargetASC, StoredPayload.AbilityLevel,
												StoredPayload.Seed, StoredPayload.AbilityTag);
		}
	}

	ExecuteZoneCue(BurstCue, ZoneLocation, 0.f);
}

// ---------------------------------------------------------------------------------------------------------------------
FVector UGeoZoneAbility::GetZoneLocation() const
{
	FVector const RotatedOffset = FRotator(0.f, StoredPayload.Yaw, 0.f).RotateVector(FVector(Offset, 0.f));
	return FVector(StoredPayload.Origin + FVector2D(RotatedOffset), ArbitraryCharacterZ);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoZoneAbility::ExecuteZoneCue(FGeoCueParam const& Cue, FVector const& ZoneLocation, float const Duration) const
{
	if (!Cue.CueTag.IsValid())
	{
		return;
	}

	FGameplayCueParameters CueParams;
	CueParams.Location = ZoneLocation;
	CueParams.Instigator = StoredPayload.Instigator;
	CueParams.AbilityLevel = StoredPayload.AbilityLevel;
	CueParams.RawMagnitude = ZoneParams.Size;
	// Same packing the patterns use: the cue reads its own timing out of Normal.
	CueParams.Normal = FVector(Duration, 0.f, 0.f);
	Cue.FillCueParams(CueParams);

	// The ability only ever runs on the server, so the cue has to be the replicated one to reach any client.
	GetAbilitySystemComponentFromActorInfo()->ExecuteGameplayCue(Cue.CueTag, CueParams);
}
