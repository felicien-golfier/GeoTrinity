// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Projectile/GeoPooledProjectile.h"

#include "Actor/Projectile/GeoProjectileFXComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "System/GeoActorPoolingSubsystem.h"

AGeoPooledProjectile::AGeoPooledProjectile()
{
	bReplicates = false;
}

void AGeoPooledProjectile::End()
{
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
	if (bIsEnding)
	{
		return;
	}
	bIsEnding = true;

	FXComponent->PlayEnd(bEndedOnValidOverlap);

	// Collision stays on until the release: the pool tells an active actor by it, and an ending shot ignores what it
	// touches anyway.
	ProjectileMovement->StopMovementImmediately();
	FXComponent->FadeOut(FSimpleDelegate::CreateWeakLambda(this,
														   [this]()
														   {
															   UGeoActorPoolingSubsystem* Pool =
																   GetWorld()->GetSubsystem<UGeoActorPoolingSubsystem>();
															   checkf(Pool, TEXT("GeoActorPoolingSubsystem is invalid!"));
															   Pool->ReleaseActor(this);
														   }));

	OnProjectileEndLifeDelegate.Broadcast(this);
}
