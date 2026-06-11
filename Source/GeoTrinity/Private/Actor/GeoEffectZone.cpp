// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/GeoEffectZone.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Tool/UGeoGameplayLibrary.h"

AGeoEffectZone::AGeoEffectZone()
{
	SetCanBeDamaged(false);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoEffectZone::OnConstruction(FTransform const& Transform)
{
	Super::OnConstruction(Transform);

 	Data.TeamID = FGenericTeamId(static_cast<uint8>(Team));
	Data.Level = Level;
	Data.Instigator = this;

	CapsuleComponent->SetCapsuleRadius(Radius);
	CapsuleComponent->SetCapsuleHalfHeight(Radius);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoEffectZone::BeginPlay()
{
	// Editor-placed: no spawner calls InitInteractable, so initialize GAS here before Super inits default attributes.
	Data.Owner = this;
	InitGas(Data.Owner);

	Super::BeginPlay();

	if (!GeoLib::IsServer(GetWorld()))
	{
		return;
	}

	ensureMsgf(EffectDataArray.Num() > 0, TEXT("AGeoEffectZone has no effects configured — it will do nothing."));

	GetWorldTimerManager().SetTimer(TickTimerHandle, this, &ThisClass::ApplyEffectsToActorsInZone, TickInterval, true);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoEffectZone::ApplyEffectsToActorsInZone()
{
	UGeoAbilitySystemComponent* SourceASC = GeoASLib::GetGeoAscFromActor(this);
	ensureMsgf(SourceASC, TEXT("AGeoEffectZone: missing ASC."));
	if (!SourceASC)
	{
		return;
	}

	TArray<AActor*> const Targets = GeoASLib::GetInteractableActors(this, GetGenericTeamId(), AttitudeBitmask, false,
																	FVector2D(GetActorLocation()), Radius);

	for (AActor* Target : Targets)
	{
		if (Target == this)
		{
			continue;
		}
		UGeoAbilitySystemComponent* TargetASC = GeoASLib::GetGeoAscFromActor(Target);
		if (!TargetASC)
		{
			continue;
		}
		GeoASLib::ApplyEffectFromEffectData(EffectDataArray, SourceASC, TargetASC, Level, Data.Seed);
	}
}
