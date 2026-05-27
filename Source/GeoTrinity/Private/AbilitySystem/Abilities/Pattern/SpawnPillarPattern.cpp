// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/SpawnPillarPattern.h"

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Deployable/Pillar/GeoPillar.h"
#include "Characters/Component/GeoDeployableManagerComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Tool/Team.h"
#include "Tool/UGeoGameplayLibrary.h"

void USpawnPillarPattern::OnCreate(FGameplayTag const AbilityTag, AActor& Owner)
{
	Super::OnCreate(AbilityTag, Owner);
	// TODO: why not having deployables in the pooling system. Need to set it up properly
	//  UGeoActorPoolingSubsystem::Get(GetWorld())->PreSpawn(PillarClass, 10);
	UGeoDeployableManagerComponent* DeployablesManagerComp =
		Owner.GetComponentByClass<UGeoDeployableManagerComponent>();
	if (DeployablesManagerComp)
	{
		DeployablesManagerComp->SetDeployableInfinitCount(PillarClass);
	}

	if (!IsValid(PillarClass))
	{
		ensureMsgf(false, TEXT("PillarClass null, please fill the class in the FatalZonePattern"));
	}
}

FGameplayCueParameters USpawnPillarPattern::FillCueParam(FAbilityPayload const& Payload)
{
	FGameplayCueParameters CueParams = Super::FillCueParam(Payload);
	CueParams.RawMagnitude = SpawningZoneSize;
	return CueParams;
}

void USpawnPillarPattern::InitPattern(FAbilityPayload const& Payload)
{
	PillarSpawnLocations.Empty();
	UGeoAbilitySystemComponent* GeoAsc = GeoASLib::GetGeoAscFromActor(Payload.Owner);
	if (!IsValid(GeoAsc))
	{
		ensureMsgf(GeoAsc, TEXT("SpawnPillarPattern: Owner has no ASC"));
		Super::InitPattern(Payload);
		return;
	}
	UGeoAttributeSetBase const* AttributeSet =
		Cast<UGeoAttributeSetBase>(GeoAsc->GetAttributeSet(UGeoAttributeSetBase::StaticClass()));

	if (!IsValid(AttributeSet))
	{
		ensureMsgf(AttributeSet, TEXT("SpawnPillarPattern: OwnerASC has no UGeoAttributeSetBase"));
		Super::InitPattern(Payload);
		return;
	}

	float const HealthRatio = AttributeSet->GetHealthRatio();
	uint8 NumPillarToSpawn = HealthRatio < .2f ? 3 : HealthRatio < .5f ? 2 : 1;
	TArray<TObjectPtr<APlayerState>> Players = GetWorld()->GetGameState()->PlayerArray;
	if (Players.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnPillarPattern: No player in the game"));
	}

	Players.Sort(
		[](TObjectPtr<APlayerState> const& A, TObjectPtr<APlayerState> const& B)
		{
			return A->GetPlayerId() < B->GetPlayerId();
		});

	for (uint8 i = 0; i < NumPillarToSpawn && i < Players.Num(); i++)
	{
		if (APawn const* TargetPawn = Players[Payload.Seed % Players.Num()]->GetPawn())
		{
			PillarSpawnLocations.Add(FVector2D(TargetPawn->GetActorLocation()));
		}
	}

	Super::InitPattern(Payload);
}
void USpawnPillarPattern::ExecuteDelayGameplayCue()
{
	bool const bIsServer = GeoLib::IsServer(GetWorld());
	if (DelayGameplayCueTag.IsValid() && !bIsServer)
	{
		UGeoAbilitySystemComponent* InstigatorASC = GeoASLib::GetGeoAscFromActor(StoredPayload.Instigator);
		if (!IsValid(InstigatorASC))
		{
			ensureMsgf(false, TEXT("Pattern Instigator %s has no ASC !"), *StoredPayload.Instigator->GetName());
		}
		else
		{
			for (FVector2D const& Location : PillarSpawnLocations)
			{
				FScopedPredictionWindow ScopedPredictionWindow(InstigatorASC);
				FGameplayCueParameters CueParams = FillCueParam(StoredPayload);
				CueParams.Location = FVector(Location, ArbitraryCharacterZ);
				InstigatorASC->ExecuteGameplayCue(DelayGameplayCueTag, CueParams);
			}
		}
	}
}

void USpawnPillarPattern::StartPattern()
{
	Super::StartPattern();
	UGeoAbilitySystemComponent* InstigatorAsc = GeoASLib::GetGeoAscFromActor(StoredPayload.Instigator);

	if (UGeoGameplayLibrary::IsServer(GetWorld()))
	{
		for (FVector2D const& Location : PillarSpawnLocations)
		{
			SpawnPillarAtLocation(Location, InstigatorAsc);
		}
	}
	EndPattern();
}

void USpawnPillarPattern::SpawnPillarAtLocation(FVector2D const& ZoneLocation,
												UGeoAbilitySystemComponent* InstigatorAsc) const
{

	if (InstigatorAsc && PillarSpawnEffects.Num() > 0)
	{
		for (AActor* TargetActor :
			 GeoASLib::GetInteractableActors(this, GeoASLib::GetTeamId(StoredPayload.Owner),
											 TeamAttitudeMask::HostileOrNeutral, true, ZoneLocation, SpawningZoneSize))
		{
			if (UGeoAbilitySystemComponent* TargetASC = GeoASLib::GetGeoAscFromActor(TargetActor))
			{
				UGeoAbilitySystemLibrary::ApplyEffectFromEffectData(PillarSpawnEffects, InstigatorAsc, TargetASC,
																	StoredPayload.AbilityLevel, StoredPayload.Seed);
			}
		}
	}

	GeoASLib::FullySpawnDeployable(PillarClass, StoredPayload, GeoASLib::GetEffectDataArray(StoredPayload.AbilityTag),
								   PillarParams, FTransform(FVector(ZoneLocation, ArbitraryCharacterZ)));
}
