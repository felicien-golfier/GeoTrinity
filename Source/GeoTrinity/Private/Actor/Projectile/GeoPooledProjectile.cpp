// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Projectile/GeoPooledProjectile.h"

#include "Components/AudioComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "System/GeoActorPoolingSubsystem.h"
#include "Tool/UGeoGameplayLibrary.h"

AGeoPooledProjectile::AGeoPooledProjectile()
{
	bReplicates = false;
}

void AGeoPooledProjectile::End()
{
	// Looping sound is cosmetic and only spawned where rendered, so stop it on every machine except the dedicated
	// server (which never started it). The listen-server host has the sound and must stop it.
	if (!GeoLib::IsDedicatedServer(this))
	{
		LoopingSoundComponent->Stop();
	}
	Sphere->OnComponentBeginOverlap.RemoveDynamic(this, &ThisClass::OnSphereOverlap);
	Sphere->OnComponentHit.RemoveDynamic(this, &ThisClass::OnSphereHit);
	UnbindFromInstigatorRevive();

	ProjectileMovement->StopMovementImmediately();
}

void AGeoPooledProjectile::Init()
{
	InitProjectileLife();
}

void AGeoPooledProjectile::EndProjectileLife()
{
	PlayImpactFx();

	UGeoActorPoolingSubsystem* Pool = GetWorld()->GetSubsystem<UGeoActorPoolingSubsystem>();
	checkf(Pool, TEXT("GeoActorPoolingSubsystem is invalid!"));
	Pool->ReleaseActor(this);

	OnProjectileEndLifeDelegate.Broadcast(this);
}
