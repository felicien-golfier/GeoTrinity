// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Deployable/Mine/GeoMine.h"

#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/GeoHexArena.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "Net/UnrealNetwork.h"
#include "Tool/UGeoGameplayLibrary.h"

AGeoMine::AGeoMine(FObjectInitializer const& ObjectInitializer) : Super(ObjectInitializer)
{
	bUseRegularDrain = true;
	bAutoRecallAtEndLife = true;
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

void AGeoMine::RecallEffect(float const Value)
{
	// Checked before Super so a mine recalled by the arena for losing its tile skips the base explode too: losing the
	// ground under a mine has to defuse it whole, or destroying the tile becomes a way to set it off.
	AGeoHexArena const* const Arena = AGeoHexArena::GetArenaOfBoss(MineData.Owner);
	if (!ensureMsgf(Arena, TEXT("AGeoMine: owner %s is not a hex arena boss"), *GetNameSafe(MineData.Owner))
		|| !Arena->IsOverAliveTile(FVector2D(GetActorLocation())))
	{
		return;
	}

	Super::RecallEffect(Value);

	if (!ensureMsgf(BurstProjectileClass, TEXT("AGeoMine: BurstProjectileClass is not set")))
	{
		return;
	}

	float const SpawnServerTime = GeoLib::GetServerTime(GetWorld());
	FAbilityPayload Payload;
	Payload.Owner = MineData.Owner;
	Payload.Instigator = this;
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

		AGeoProjectile* const Projectile = GeoASLib::StartSpawnProjectile(GetWorld(), BurstProjectileClass,
																		  SpawnTransform, Payload, MineData.EffectDataArray);
		if (!ensureMsgf(Projectile, TEXT("AGeoMine: failed to spawn burst projectile")))
		{
			return;
		}
		GeoASLib::FinishSpawnProjectile(GetWorld(), Projectile, SpawnTransform, SpawnServerTime, FPredictionKey());
	}
}
