// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Common/GeoDeployAbility.h"

#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystemComponent.h"
#include "Actor/Projectile/DeployableSpawner/DeployableSpawnerProjectile.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "Characters/Component/GeoDeployableManagerComponent.h"
#include "Settings/GameDataSettings.h"
#include "Tool/UGeoGameplayLibrary.h"

UGeoDeployAbility::UGeoDeployAbility()
{
	FireMode = EFireMode::ChargeForFireDelay;
	bOverrideSpeed = true;
	ProjectileSpeed = 2000.f;
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoDeployAbility::CanActivateAbility(FGameplayAbilitySpecHandle const Handle,
										   FGameplayAbilityActorInfo const* ActorInfo,
										   FGameplayTagContainer const* SourceTags,
										   FGameplayTagContainer const* TargetTags,
										   FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	UGeoDeployableManagerComponent* DeployableManager =
		ActorInfo->AvatarActor->GetComponentByClass<UGeoDeployableManagerComponent>();
	ensureMsgf(DeployableManager, TEXT("GeoDeployAbility: No UGeoDeployableManagerComponent on avatar '%s'!"),
			   *GetNameSafe(ActorInfo->AvatarActor.Get()));
	if (!DeployableManager)
	{
		return false;
	}

	return DeployableManager->CanDeploy(DeployableActorClass);
}

// ---------------------------------------------------------------------------------------------------------------------
FGeoAbilityTargetData UGeoDeployAbility::GetUpdatedTargetData()
{
	UGameDataSettings const* GameDataSettings = GetDefault<UGameDataSettings>();
	float PendingDeployDistance =
		FMath::Lerp(GameDataSettings->MinDeployDistance, GameDataSettings->MaxDeployDistance, GetChargeRatio());
	// Encode deploy distance as integer cm in Seed so the server receives it
	StoredPayload.Seed = FMath::RoundToInt(PendingDeployDistance);
	return Super::GetUpdatedTargetData();
}


// ---------------------------------------------------------------------------------------------------------------------
void UGeoDeployAbility::SpawnProjectile(FTransform const& SpawnTransform, float const SpawnServerTime) const
{
	checkf(ProjectileClass, TEXT("No ProjectileClass set on GeoDeployAbility!"));

	FPredictionKey PredictionKey;
	EGameplayAbilityActivationMode::Type const ActivationMode = GetCurrentActivationInfo().ActivationMode;
	if (ActivationMode == EGameplayAbilityActivationMode::Predicting
		|| ActivationMode == EGameplayAbilityActivationMode::Confirmed
		|| ActivationMode == EGameplayAbilityActivationMode::Authority)
	{
		PredictionKey = GetCurrentActivationInfo().GetActivationPredictionKey();
	}

	AGeoProjectile* Projectile = GeoASLib::StartSpawnProjectile(GetWorld(), ProjectileClass, SpawnTransform,
																StoredPayload, GetEffectDataArray(), PredictionKey);
	if (!IsValid(Projectile))
	{
		ensureMsgf(false, TEXT("GeoDeployAbility: Failed to spawn projectile!"));
		return;
	}

	Projectile->OverrideDistanceSpan(StoredPayload.Seed);
	ADeployableSpawnerProjectile* DeployableSpawnerProjectile = Cast<ADeployableSpawnerProjectile>(Projectile);
	checkf(DeployableSpawnerProjectile, TEXT("SpawnerProjectile  must be a ADeployableSpawnerProjectile"));
	DeployableSpawnerProjectile->Params = Params;
	DeployableSpawnerProjectile->DeployableActorClass = DeployableActorClass;
	if (bOverrideDistanceSpan)
	{
		Projectile->OverrideDistanceSpan(DistanceSpan);
	}
	if (bOverrideSpeed)
	{
		Projectile->OverrideSpeed(ProjectileSpeed);
	}

	GeoASLib::FinishSpawnProjectile(GetWorld(), Projectile, SpawnTransform, SpawnServerTime, PredictionKey);
}
