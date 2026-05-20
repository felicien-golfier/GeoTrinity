// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/FatalZonePattern.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Deployable/GeoDeployableBase.h"
#include "Actor/Deployable/Pillar/GeoPillar.h"
#include "System/GeoActorPoolingSubsystem.h"
#include "System/GeoPoolableInterface.h"
#include "Tool/Team.h"
#include "Tool/UGeoGameplayLibrary.h"

void UFatalZonePattern::OnCreate(FGameplayTag AbilityTag)
{
	Super::OnCreate(AbilityTag);
	// TODO: why not having deployables in the pooling system. Need to set it up properly
	//  UGeoActorPoolingSubsystem::Get(GetWorld())->PreSpawn(PillarClass, 10);
}

FGameplayCueParameters UFatalZonePattern::FillCueParam(FAbilityPayload const& Payload)
{
	FGameplayCueParameters CueParams = Super::FillCueParam(Payload);
	CueParams.RawMagnitude = ZoneSize;
	return CueParams;
}

void UFatalZonePattern::StartPattern()
{
	Super::StartPattern();

	FVector const ZoneLocation = FVector(StoredPayload.Origin, ArbitraryCharacterZ);
	UGeoAbilitySystemComponent* InstigatorAsc = GeoASLib::GetGeoAscFromActor(StoredPayload.Instigator);

	UWorld* World = GetWorld();
	if (UGeoGameplayLibrary::IsServer(World))
	{
		if (InstigatorAsc && ZoneEffectDataArray.Num() > 0)
		{
			for (AActor* TargetActor : GeoASLib::GetInteractableActors(this, GeoASLib::GetTeamId(StoredPayload.Owner),
																	   TeamAttitudeMask::HostileOrNeutral, true,
																	   StoredPayload.Origin, ZoneSize))
			{
				if (UGeoAbilitySystemComponent* TargetASC = GeoASLib::GetGeoAscFromActor(TargetActor))
				{
					UGeoAbilitySystemLibrary::ApplyEffectFromEffectData(ZoneEffectDataArray, InstigatorAsc, TargetASC,
																		StoredPayload.AbilityLevel, StoredPayload.Seed);
				}
			}
		}

		if (!IsValid(PillarClass))
		{
			ensureMsgf(false, TEXT("PillarClass null, please fill the class in the FatalZonePattern"));
			return;
		}

		AGeoPillar* Pillar = Cast<AGeoPillar>(UGeoActorPoolingSubsystem::Get(World)->RequestActor(
			PillarClass, FTransform(ZoneLocation), StoredPayload.Owner, Cast<APawn>(StoredPayload.Instigator), false,
			false));

		if (IsValid(Pillar))
		{
			FDeployableData PillarData;
			PillarData.Owner = StoredPayload.Owner;
			PillarData.Instigator = StoredPayload.Instigator;
			PillarData.Level = StoredPayload.AbilityLevel;
			PillarData.Seed = StoredPayload.Seed;
			PillarData.EffectDataArray = GeoASLib::GetEffectDataArray(StoredPayload.AbilityTag);
			PillarData.Params.Size = ZoneSize;

			Pillar->InitInteractable(&PillarData);
			UGeoActorPoolingSubsystem::Get(World)->ChangeActorState(Pillar, true);
			if (Pillar->GetClass()->ImplementsInterface(UGeoPoolableInterface::StaticClass()))
			{
				CastChecked<IGeoPoolableInterface>(Pillar)->Init();
			}
		}
	}

	EndPattern();
}
