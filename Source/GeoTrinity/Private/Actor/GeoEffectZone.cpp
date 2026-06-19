// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/GeoEffectZone.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Tool/UGeoGameplayLibrary.h"

AGeoEffectZone::AGeoEffectZone()
{
	PrimaryActorTick.bCanEverTick = true;
	CapsuleComponent->SetCapsuleRadius(Radius);
	CapsuleComponent->SetCapsuleHalfHeight(Radius);
	SetCanBeDamaged(false);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoEffectZone::OnConstruction(FTransform const& Transform)
{
	Super::OnConstruction(Transform);

	CapsuleComponent->SetCapsuleRadius(Radius);
	CapsuleComponent->SetCapsuleHalfHeight(Radius);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoEffectZone::BeginPlay()
{
	// Editor-placed: no spawner calls InitInteractable, so initialize GAS here before Super inits default attributes.
	Data.Owner = this;
	Data.TeamID = FGenericTeamId(static_cast<uint8>(Team));
	Data.Level = Level;
	Data.Instigator = this;
	InitGas(Data.Owner);

	Super::BeginPlay();

	if (!GeoLib::IsServer(GetWorld()))
	{
		return;
	}

	ensureMsgf(EffectDataArray.Num() > 0, TEXT("AGeoEffectZone has no effects configured — it will do nothing."));

	CapsuleComponent->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnBeginOverlap);
	CapsuleComponent->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnEndOverlap);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoEffectZone::OnBeginOverlap(UPrimitiveComponent* /*OverlappedComponent*/, AActor* OtherActor,
									UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/,
									FHitResult const& /*SweepResult*/)
{
	UGeoAbilitySystemComponent* TargetASC = GeoASLib::GetGeoAscFromActor(OtherActor);
	if (!TargetASC || OtherActor == this || !GeoASLib::IsTeamAttitudeAligned(this, OtherActor, AttitudeBitmask))
	{
		return;
	}

	UGeoAbilitySystemComponent* SourceASC = GeoASLib::GetGeoAscFromActor(this);
	ensureMsgf(SourceASC, TEXT("AGeoEffectZone: missing ASC."));
	if (!SourceASC)
	{
		return;
	}

	// Persistent (non heal/damage) effects are applied once on entry and removed on exit; their handles are stored.
	TArray<FActiveGameplayEffectHandle>& Handles = ActorsInZone.Add(OtherActor);
	for (TInstancedStruct<FEffectData> const& Entry : EffectDataArray)
	{
		// Heal/damage entries tick in Tick(); everything else is a persistent effect applied once here.
		if (Entry.GetPtr<FHealEffectData>() || Entry.GetPtr<FDamageEffectData>())
		{
			continue;
		}
		Handles.Add(GeoASLib::ApplySingleEffectData(Entry, SourceASC, TargetASC, Level, Data.Seed));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoEffectZone::OnEndOverlap(UPrimitiveComponent* /*OverlappedComponent*/, AActor* OtherActor,
								  UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/)
{
	TArray<FActiveGameplayEffectHandle> Handles;
	if (!ActorsInZone.RemoveAndCopyValue(OtherActor, Handles))
	{
		return;
	}

	UGeoAbilitySystemComponent* TargetASC = GeoASLib::GetGeoAscFromActor(OtherActor);
	if (!TargetASC)
	{
		return;
	}
	for (FActiveGameplayEffectHandle const& Handle : Handles)
	{
		TargetASC->RemoveActiveGameplayEffect(Handle);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoEffectZone::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!GeoLib::IsServer(GetWorld()) || ActorsInZone.IsEmpty())
	{
		return;
	}

	UGeoAbilitySystemComponent* SourceASC = GeoASLib::GetGeoAscFromActor(this);
	if (!SourceASC)
	{
		return;
	}

	for (auto const& Pair : TMap<TWeakObjectPtr<AActor>, TArray<FActiveGameplayEffectHandle>>{ActorsInZone})
	{
		AActor* Actor = Pair.Key.Get();
		UGeoAbilitySystemComponent* TargetASC = GeoASLib::GetGeoAscFromActor(Actor);
		if (!TargetASC)
		{
			continue;
		}

		for (TInstancedStruct<FEffectData> const& Entry : EffectDataArray)
		{
			if (!IsValid(Actor) || !IsValid(TargetASC) || !Actor->CanBeDamaged())
			{
				break; // Do not continue if the actor is dead or destroyed during this loop.
			}

			if (FHealEffectData const* Heal = Entry.GetPtr<FHealEffectData>())
			{
				FHealEffectData Scaled = *Heal;
				Scaled.HealAmount = Heal->HealAmount.GetValueAtLevel(Level) * DeltaSeconds;
				GeoASLib::ApplySingleEffectData(Scaled, SourceASC, TargetASC, Level, Data.Seed);
			}
			else if (FDamageEffectData const* Damage = Entry.GetPtr<FDamageEffectData>())
			{
				FDamageEffectData Scaled = *Damage;
				Scaled.DamageAmount = Damage->DamageAmount.GetValueAtLevel(Level) * DeltaSeconds;
				GeoASLib::ApplySingleEffectData(Scaled, SourceASC, TargetASC, Level, Data.Seed);
			}
		}
	}
}
