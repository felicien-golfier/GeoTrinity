// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/SpiralPattern.h"

#include "Actor/Projectile/GeoProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "System/GeoActorPoolingSubsystem.h"
#include "System/GeoPoolableInterface.h"
#include "Tool/UGeoGameplayLibrary.h"

void USpiralPattern::OnCreate(FGameplayTag AbilityTag, AActor& Owner)
{
	Super::OnCreate(AbilityTag, Owner);

	float const MaxProjectileNum = RoundNumber * NumberProjectileByRound;
	ensureMsgf(MaxProjectileNum > 0, TEXT("No projectile set in the spiral ! please fill your pattern values in BP"));
	Projectiles.Reserve(MaxProjectileNum);

	if (!ensureMsgf(ProjectileParams.ProjectileClass,
					TEXT("You must fill the projectile class in the Spiral pattern.")))
	{
		return;
	}
	ProjectileSpeed =
		ProjectileParams.ProjectileClass->GetDefaultObject<AGeoProjectile>()->ProjectileMovement->InitialSpeed;
	TimeDiffBetweenProjectiles = TimeForOneRound / NumberProjectileByRound;
	AngleBetweenProjectiles = 360.f / NumberProjectileByRound;

	UGeoActorPoolingSubsystem::Get(GetWorld())
		->PreSpawn(ProjectileParams.ProjectileClass,
				   MaxProjectileNum * 0.67f /*Create a first estimation number of projectiles in the pool.*/);
}

void USpiralPattern::InitPattern(FAbilityPayload const& Payload, TInstancedStruct<FPatternData> const& PatternData)
{
	Super::InitPattern(Payload, PatternData);

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
		if (!bPatternIsActive) // Cuz a projectile can kill during the loop
		{
			return;
		}

		FVector const ProjectileDirection =
			FirstProjectileOrientation.RotateAngleAxis(i * AngleBetweenProjectiles, FVector::UpVector);

		FVector ProjectileLocation = FVector(StoredPayload.Origin, ArbitraryCharacterZ)
			+ ProjectileDirection * ProjectileSpeed * (SpentTime - i * TimeDiffBetweenProjectiles);

		if (Projectiles.Num() <= i)
		{
			UGeoActorPoolingSubsystem* Pooling = UGeoActorPoolingSubsystem::Get(GetWorld());
			AGeoProjectile* Projectile =
				Pooling->RequestActor(ProjectileParams.ProjectileClass,
									  FTransform(ProjectileDirection.ToOrientationRotator(), ProjectileLocation),
									  StoredPayload.Owner, Cast<APawn>(StoredPayload.Instigator), false, false);

			Projectile->Payload = StoredPayload;
			Projectile->EffectDataArray = EffectDataArray;
			Projectile->OnProjectileEndLifeDelegate.AddUniqueDynamic(this, &USpiralPattern::EndProjectile);
			Projectile->ApplyProjectileParams(ProjectileParams);
			Projectiles.Add(Projectile);

			Pooling->ChangeActorState(Projectile, true);
			if (ProjectileParams.ProjectileClass->ImplementsInterface(UGeoPoolableInterface::StaticClass()))
			{
				CastChecked<IGeoPoolableInterface>(Projectile)->Init();
			}
		}

		AGeoProjectile* Projectile = Projectiles[i];
		if (!IsValid(Projectile))
		{
			// Projectile has been ended.
			continue;
		}

		Projectile->SetActorRotation(ProjectileDirection.Rotation());
		Projectile->SetActorLocation(ProjectileLocation);

		if (!IsValid(Projectile))
		{
			// Projectile May be ended after moving it.
			continue;
		}

		bHasValidProjectiles = true;
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

void USpiralPattern::EndPattern(bool bForceStop)
{
	Super::EndPattern(bForceStop);
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
