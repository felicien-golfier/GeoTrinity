// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Projectile/DeployableSpawner/DeployableSpawnerProjectile.h"

#include "Characters/Component/GeoDeployableManagerComponent.h"
#include "GameFramework/PlayerState.h"
#include "GenericTeamAgentInterface.h"

// ---------------------------------------------------------------------------------------------------------------------
bool ADeployableSpawnerProjectile::IsValidOverlap(AActor const* OtherActor)
{
	return OtherActor != GetInstigator() && Super::IsValidOverlap(OtherActor);
}

// ---------------------------------------------------------------------------------------------------------------------
void ADeployableSpawnerProjectile::EndProjectileLife()
{
	Super::EndProjectileLife();
	SpawnDeployableActor();
}

// ---------------------------------------------------------------------------------------------------------------------
void ADeployableSpawnerProjectile::FillBaseData(FDeployableData& Data, AActor* PayloadOwner) const
{
	Data.CharacterOwner = PayloadOwner;
	Data.Level = Payload.AbilityLevel;
	Data.MaxDuration = LifeDrain;
	if (IGenericTeamAgentInterface const* TeamInterface = Cast<IGenericTeamAgentInterface>(PayloadOwner))
	{
		Data.TeamID = TeamInterface->GetGenericTeamId();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void ADeployableSpawnerProjectile::InitDeployable(AGeoDeployableBase* Deployable, AActor* PayloadOwner) const
{
	FDeployableData Data;
	FillBaseData(Data, PayloadOwner);
	Deployable->InitInteractableData(&Data);
}

// ---------------------------------------------------------------------------------------------------------------------
void ADeployableSpawnerProjectile::SpawnDeployableActor() const
{
	if (!UGameplayLibrary::IsServer(GetWorld()))
	{
		return;
	}

	AActor* PayloadOwner = Payload.Owner;
	ensureMsgf(IsValid(PayloadOwner), TEXT("DeployableSpawnerProjectile: Payload.Owner must exist on server"));
	if (!IsValid(PayloadOwner))
	{
		return;
	}

	checkf(DeployableActorClass, TEXT("DeployableSpawnerProjectile: No DeployableActorClass set!"));

	APawn* Pawn = Cast<APawn>(PayloadOwner);
	if (!IsValid(Pawn))
	{
		if (APlayerState const* PlayerState = Cast<APlayerState>(PayloadOwner))
		{
			Pawn = PlayerState->GetPawn();
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("DeployableSpawnerProjectile: No valid pawn to spawn deployable!"));
			return;
		}
	}

	FTransform const SpawnTransform = GetActorTransform();
	AGeoDeployableBase* Deployable = GetWorld()->SpawnActorDeferred<AGeoDeployableBase>(
		DeployableActorClass, SpawnTransform, PayloadOwner, Pawn, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (!IsValid(Deployable))
	{
		UE_LOG(LogTemp, Error, TEXT("DeployableSpawnerProjectile: Failed to spawn deployable!"));
		return;
	}

	InitDeployable(Deployable, PayloadOwner);
	Deployable->FinishSpawning(SpawnTransform);

	if (UGeoDeployableManagerComponent* DeployableManager = Pawn->GetComponentByClass<UGeoDeployableManagerComponent>())
	{
		DeployableManager->RegisterDeployable(Deployable);
	}
}
