// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/SpiralPattern.h"

#include "Actor/Projectile/GeoProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "System/GeoActorPoolingSubsystem.h"
#include "System/GeoPoolableInterface.h"

void USpiralPattern::OnCreate(FGameplayTag AbilityTag)
{
	Super::OnCreate(AbilityTag);

	float const MaxProjectileNum = RoundNumber * NumberProjectileByRound;
	ensureMsgf(MaxProjectileNum > 0, TEXT("No projectile set in the spiral ! please fill your pattern values in BP"));
	Projectiles.Reserve(MaxProjectileNum);

	ensureMsgf(ProjectileClass, TEXT("You must fill the projectile class in the Spiral pattern."));
	ProjectileSpeed = ProjectileClass->GetDefaultObject<AGeoProjectile>()->ProjectileMovement->InitialSpeed;
	TimeDiffBetweenProjectiles = TimeForOneRound / NumberProjectileByRound;
	AngleBetweenProjectiles = 360.f / NumberProjectileByRound;

	UGeoActorPoolingSubsystem::Get(GetWorld())
		->PreSpawn(ProjectileClass,
				   MaxProjectileNum * 0.67f /*Create a first estimation number of projectiles in the pool.*/);
}

void USpiralPattern::InitPattern(FAbilityPayload const& Payload)
{
	Super::InitPattern(Payload);

	// Map Seed (full int32 range) to [0, 360] to give each spiral instance a deterministic but unique
	// starting angle, so two spirals fired with different seeds don't overlap identically.
	FirstProjectileOrientation =
		FVector(1.f, 1.f, 0.f)
			.RotateAngleAxis((static_cast<float>(Payload.Seed) / MAX_int32) * 360.f, FVector::UpVector);
}

void USpiralPattern::TickPattern(float const ServerTime, float const SpentTime)
{
	int const ProjectileNumSpawned =
		FMath::Floor(1 + SpentTime / TimeDiffBetweenProjectiles); // 1+ because first projectile spawns at 0.
	bool bHasValidProjectiles = false;

	for (int i = 0; i < RoundNumber * NumberProjectileByRound && i < ProjectileNumSpawned; i++)
	{
		if (Projectiles.Num() <= i)
		{

			AGeoProjectile* Projectile = UGeoActorPoolingSubsystem::Get(GetWorld())
											 ->RequestActor(ProjectileClass, FTransform::Identity, StoredPayload.Owner,
															Cast<APawn>(StoredPayload.Instigator), false, false);

			Projectile->Payload = StoredPayload;
			Projectile->EffectDataArray = EffectDataArray;
			Projectiles.Add(Projectile);
			Projectile->OnProjectileEndLifeDelegate.AddUniqueDynamic(this, &USpiralPattern::EndProjectile);
		}

		AGeoProjectile* Projectile = Projectiles[i];
		if (!IsValid(Projectile))
		{
			// Projectile has been ended.
			continue;
		}

		bHasValidProjectiles = true;

		FVector const ProjectileDirection =
			FirstProjectileOrientation.RotateAngleAxis(i * AngleBetweenProjectiles, FVector::UpVector);
		Projectile->SetActorRotation(ProjectileDirection.Rotation());

		FVector ProjectileLocation = FVector(StoredPayload.Origin, 0.f)
			+ ProjectileDirection * ProjectileSpeed * (SpentTime - i * TimeDiffBetweenProjectiles);
		Projectile->SetActorLocation(ProjectileLocation);

		// Activate the projectile on the first tick that catches up to its spawn time.
		// Checking the state prevents calling Init() again if we're re-ticking an already-active projectile.
		if (!UGeoActorPoolingSubsystem::Get(GetWorld())->GetActorState(Projectile))
		{
			UGeoActorPoolingSubsystem::Get(GetWorld())->ChangeActorState(Projectile, true);
			if (ProjectileClass->ImplementsInterface(UGeoPoolableInterface::StaticClass()))
			{
				CastChecked<IGeoPoolableInterface>(Projectile)->Init();
			}
		}
	}

	if (ProjectileNumSpawned >= RoundNumber * NumberProjectileByRound)
	{
		JumpMontageToEndSection();
	}

	if (!bHasValidProjectiles && ProjectileNumSpawned > 0)
	{
		EndPattern();
	}
}

void USpiralPattern::EndPattern()
{
	Super::EndPattern();
	for (int i = 0; i < Projectiles.Num(); i++)
	{
		if (IsValid(Projectiles[i]))
		{
			EndProjectile(Projectiles[i]);
		}
	}
	Projectiles.Empty();
}

void USpiralPattern::EndProjectile(AGeoProjectile* Projectile)
{
	ensureMsgf(IsValid(Projectile), TEXT("Projectile is invalid !"));
	for (int i = 0; i < Projectiles.Num(); i++)
	{
		if (Projectiles[i] == Projectile)
		{
			if (IsValid(Projectile))
			{
				Projectile->OnProjectileEndLifeDelegate.RemoveDynamic(this, &USpiralPattern::EndProjectile);
			}
			Projectiles[i] = nullptr;
		}
	}
}
