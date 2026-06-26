// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Boss/GeoSpawnPillarAbility.h"

#include "AbilitySystem/Abilities/Pattern/SpawnPillarPattern.h"
#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"

TInstancedStruct<FPatternData> UGeoSpawnPillarAbility::CreatePatternData() const
{
	FSpawnPillarPatternData PillarData;

	UGeoAbilitySystemComponent* GeoAsc = GeoASLib::GetGeoAscFromActor(StoredPayload.Owner);
	if (!ensureMsgf(IsValid(GeoAsc), TEXT("GeoSpawnPillarAbility: Owner has no ASC")))
	{
		return TInstancedStruct<FPatternData>::Make<FSpawnPillarPatternData>(PillarData);
	}

	UGeoAttributeSetBase const* AttributeSet =
		Cast<UGeoAttributeSetBase>(GeoAsc->GetAttributeSet(UGeoAttributeSetBase::StaticClass()));
	if (!ensureMsgf(IsValid(AttributeSet), TEXT("GeoSpawnPillarAbility: OwnerASC has no UGeoAttributeSetBase")))
	{
		return TInstancedStruct<FPatternData>::Make<FSpawnPillarPatternData>(PillarData);
	}

	float const HealthRatio = AttributeSet->GetHealthRatio();
	int NumPillarToSpawn = HealthRatio < .2f ? 3 : HealthRatio < .5f ? 2 : 1;

	TArray<TObjectPtr<APlayerState>> Players = GetWorld()->GetGameState()->PlayerArray;
	if (Players.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("GeoSpawnPillarAbility: No player in the game"));
	}

	TArray<APawn*> AlivePawns;
	TArray<APawn*> DeadPawns;

	for (auto Player : Players)
	{
		if (IsValid(Player))
		{
			APawn* TargetPawn = Player->GetPawn();
			if (IsValid(TargetPawn))
			{
				if (TargetPawn->CanBeDamaged())
				{
					AlivePawns.Add(TargetPawn);
				}
				else
				{
					DeadPawns.Add(TargetPawn);
				}
			}
		}
	}

	NumPillarToSpawn = FMath::Min(NumPillarToSpawn, AlivePawns.Num() + DeadPawns.Num());
	for (uint8 i = 0; i < NumPillarToSpawn && i < Players.Num(); i++)
	{
		if (PillarData.ZoneLocations.Num() < AlivePawns.Num())
		{
			PillarData.ZoneLocations.Add(
				FVector2D(AlivePawns[(StoredPayload.Seed % AlivePawns.Num() + i) % AlivePawns.Num()]->GetActorLocation()));
		}
		else
		{
			PillarData.ZoneLocations.Add(
				FVector2D(DeadPawns[(StoredPayload.Seed % DeadPawns.Num() + i) % DeadPawns.Num()]->GetActorLocation()));
		}
	}

	return TInstancedStruct<FPatternData>::Make<FSpawnPillarPatternData>(PillarData);
}
