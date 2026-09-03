// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Deployable/Mine/GeoMine.h"

#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "Net/UnrealNetwork.h"
#include "Tool/UGeoGameplayLibrary.h"

AGeoMine::AGeoMine(FObjectInitializer const& ObjectInitializer) : Super(ObjectInitializer)
{
	bUseRegularDrain = false;
	bAutoRecallAtEndLife = true;
	bExplodeAtRecall = true;
	bDamageableDuringBlink = true;
	bShowDamageNumbers = false;

	OutlineColor = EGeoColor::DeployableBlockingEnemies;
}

void AGeoMine::InitInteractable(FInteractableActorData* Data)
{
	FDeployableData* const DeployableData = static_cast<FDeployableData*>(Data);
	if (!ensureMsgf(DeployableData, TEXT("AGeoMine: Data is not a FDeployableData!")))
	{
		return;
	}
	MineData = *DeployableData;

	Super::InitInteractable(Data);
}

void AGeoMine::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AGeoMine, MineData, COND_InitialOnly);
}

void AGeoMine::InitDrain()
{
	if (!GeoLib::IsServer(GetWorld()) || MineData.Params.LifeDrainMaxDuration <= 0.f)
	{
		return;
	}

	GetWorldTimerManager().SetTimer(FuseTimerHandle, this, &AGeoMine::OnFuseElapsed,
									MineData.Params.LifeDrainMaxDuration, false);
}

void AGeoMine::OnHealthChanged_Implementation(float const NewValue)
{
	if (NewValue <= 0.f && bActive)
	{
		Expire();
	}
}

void AGeoMine::OnFuseElapsed()
{
	if (!bActive)
	{
		return;
	}

	if (ensureMsgf(MineData.Params.BlinkDuration > 0.f, TEXT("A Mine without Blink duration is not readable")))
	{
		StartBlinking();
	}
}

void AGeoMine::ExplodeEffect(float const Value)
{
	// Don't call super, cuz we don;t want to apply effects on the zone.
	if (!ensureMsgf(ProjectileParams.ProjectileClass, TEXT("AGeoMine: ProjectileParams.ProjectileClass is not set")))
	{
		return;
	}

	float const SpawnServerTime = GeoLib::GetServerTime(GetWorld());
	FAbilityPayload Payload;
	Payload.SourceOwner = MineData.Owner;
	Payload.SourceAvatar = this;
	Payload.Origin = FVector2D(GetActorLocation());
	Payload.ServerSpawnTime = SpawnServerTime;
	Payload.AbilityLevel = MineData.Level;
	Payload.AbilityTag = MineData.AbilityTag;
	Payload.Seed = MineData.Seed;

	float const AngleBetweenProjectiles = 360.f / BurstProjectileCount;
	for (int32 Index = 0; Index < BurstProjectileCount; ++Index)
	{
		Payload.Yaw = AngleBetweenProjectiles * Index;
		FTransform const SpawnTransform(FRotator(0.f, Payload.Yaw, 0.f), GetActorLocation());

		AGeoProjectile* const Projectile = GeoASLib::StartSpawnProjectile(GetWorld(), ProjectileParams, SpawnTransform,
																		  Payload, MineData.EffectDataArray);
		if (!ensureMsgf(Projectile, TEXT("AGeoMine: failed to spawn burst projectile")))
		{
			return;
		}
		GeoASLib::FinishSpawnProjectile(GetWorld(), Projectile, SpawnTransform, SpawnServerTime, FPredictionKey());
	}
}
