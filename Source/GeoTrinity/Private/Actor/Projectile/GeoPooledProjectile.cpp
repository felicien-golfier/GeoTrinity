// Fill out your copyright notice in the Description page of Project Settings.

#include "Actor/Projectile/GeoPooledProjectile.h"

#include "Components/AudioComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "System/GeoActorPoolingSubsystem.h"

AGeoPooledProjectile::AGeoPooledProjectile()
{
	bReplicates = false;
}

void AGeoPooledProjectile::End()
{
	if (!HasAuthority())
	{
		LoopingSoundComponent->Stop();
	}
	Sphere->OnComponentBeginOverlap.RemoveDynamic(this, &ThisClass::OnSphereOverlap);
	Sphere->OnComponentHit.RemoveDynamic(this, &ThisClass::OnSphereHit);

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
