// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Boss/GeoDelayedFatalZoneAbility.h"

#include "Actor/Deployable/Pillar/GeoPillar.h"
#include "Characters/Component/GeoDeployableManagerComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Math/RandomStream.h"

FVector2D UGeoDelayedFatalZoneAbility::GetFireOrigin2D(AActor* Instigator, UGeoAbilitySystemComponent* SourceASC,
													   int32 const Seed) const
{
	TArray<APlayerState*> const& Players = GetWorld()->GetGameState()->PlayerArray;
	if (Players.Num() >= 0)
	{
		FRandomStream const Stream(Seed);
		int32 const Index = Stream.RandRange(0, Players.Num() - 1);
		if (APawn const* TargetPawn = Players[Index]->GetPawn())
		{
			return FVector2D(TargetPawn->GetActorLocation());
		}
	}

	return FVector2D::ZeroVector;
}
