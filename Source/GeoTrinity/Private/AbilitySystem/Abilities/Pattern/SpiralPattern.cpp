// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Pattern/SpiralPattern.h"

#include "Actor/Projectile/GeoProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "System/GeoActorPoolingSubsystem.h"

void USpiralPattern::OnCreate(FGameplayTag AbilityTag)
{
	Super::OnCreate(AbilityTag);

	const float MaxProjectileNum = RoundNumber * NumberProjectileByRound;
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

void USpiralPattern::InitPattern(const FAbilityPayload& Payload)
{
	Super::InitPattern(Payload);

	FirstProjectileOrientation =
		FVector(1.f, 1.f, 0.f)
			.RotateAngleAxis((static_cast<float>(Payload.Seed) / MAX_int32) * 360.f, FVector::UpVector);
}

void USpiralPattern::TickPattern(const float ServerTime, const float SpentTime)
{
	const int ProjectileNumSpawned =
		FMath::Floor(1 + SpentTime / TimeDiffBetweenProjectiles);   // 1+ because first projectile spawns at 0.
	bool bHasValidProjectiles = false;

	for (int i = 0; i < RoundNumber * NumberProjectileByRound && i < ProjectileNumSpawned; i++)
	{
		if (Projectiles.Num() <= i)
		{

			AGeoProjectile* Projectile = UGeoActorPoolingSubsystem::Get(GetWorld())
			                                 ->RequestActor(ProjectileClass, FTransform::Identity, StoredPayload.Owner,
												 Cast<APawn>(StoredPayload.Instigator), false, false);

			Projectile->Payload = StoredPayload;
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

		const FVector ProjectileDirection =
			FirstProjectileOrientation.RotateAngleAxis(i * AngleBetweenProjectiles, FVector::UpVector);
		Projectile->SetActorRotation(ProjectileDirection.Rotation());

		FVector ProjectileLocation =
			FVector(StoredPayload.Origin, 0.f)
			+ ProjectileDirection * ProjectileSpeed * (SpentTime - i * TimeDiffBetweenProjectiles);
		Projectile->SetActorLocation(ProjectileLocation);

		if (!UGeoActorPoolingSubsystem::Get(GetWorld())->GetActorState(Projectile))
		{
			UGeoActorPoolingSubsystem::Get(GetWorld())->ChangeActorState(Projectile, true);
			Projectile->Init();
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