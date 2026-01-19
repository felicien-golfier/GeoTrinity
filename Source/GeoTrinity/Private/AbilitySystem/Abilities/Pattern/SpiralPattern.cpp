// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Pattern/SpiralPattern.h"

#include "Actor/Projectile/GeoProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GeoPlayerController.h"
#include "System/GeoActorPoolingSubsystem.h"
#include "Tool/GameplayLibrary.h"
void USpiralPattern::StartPattern_Implementation(const FAbilityPayload& Payload)
{
	Super::StartPattern_Implementation(Payload);
	float MaxProjectileNum = RoundNumber * NumberProjectileByRound;
	checkf(MaxProjectileNum > 0, TEXT("No projectile set in the spiral ! please fill your pattern values in BP"));
	Projectiles.Reserve(MaxProjectileNum);
	for (int i = 0; i < RoundNumber * NumberProjectileByRound; i++)
	{
		AGeoProjectile* Projectile = UGeoActorPoolingSubsystem::Get(GetWorld())
		                                 ->RequestActor(ProjectileClass, FTransform::Identity, Payload.Owner,
											 Cast<APawn>(Payload.Instigator), false, false);

		Projectile->Payload = Payload;
		Projectiles.Add(Projectile);
		Projectile->OnProjectileEndLifeDelegate.AddUniqueDynamic(this, &USpiralPattern::EndProjectile);
	}

	ProjectileSpeed = ProjectileClass->GetDefaultObject<AGeoProjectile>()->ProjectileMovement->InitialSpeed;
	TimeDiffBetweenProjectiles = TimeForOneRound / NumberProjectileByRound;
	AngleBetweenProjectiles = 360.f / NumberProjectileByRound;
	FirstProjectileOrientation =
		FVector(1.f, 1.f, 0.f).RotateAngleAxis(static_cast<float>(Payload.Seed) / MAX_int32, FVector::UpVector);
}

void USpiralPattern::TickPattern(float DeltaSeconds)
{
	const double CurrentServerTime = GameplayLibrary::GetServerTime(GetWorld());
	const double SpentTime = CurrentServerTime - StoredPayload.ServerSpawnTime;
	const float ProjectileNumSpawned =
		1 + SpentTime / TimeDiffBetweenProjectiles;   // 1+ because first projectile spawns at 0.

	bool bHasValidProjectiles = false;
	for (int i = 0; i < Projectiles.Num() && i < FMath::Floor(ProjectileNumSpawned); i++)
	{
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

	if (!bHasValidProjectiles)
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
	checkf(IsValid(Projectile), TEXT("Projectile is invalid !"));
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