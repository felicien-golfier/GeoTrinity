// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Deployable/Zones/GeoEffectZone.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Components/MeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "Tool/UGeoGameplayLibrary.h"

AGeoEffectZone::AGeoEffectZone(FObjectInitializer const& ObjectInitializer) : Super(ObjectInitializer)
{
	bShowDamageNumbers = false;
	SetCanBeDamaged(false);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoEffectZone::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AGeoEffectZone, Data, COND_InitialOnly);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoEffectZone::InitInteractable(FInteractableActorData* InputData)
{
	FDeployableData* const DeployableData = static_cast<FDeployableData*>(InputData);
	if (!ensureMsgf(DeployableData, TEXT("AGeoEffectZone: Data is not an FDeployableData!")))
	{
		return;
	}
	Data = *DeployableData;
	ApplyRadius();

	Super::InitInteractable(InputData);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoEffectZone::OnConstruction(FTransform const& Transform)
{
	Super::OnConstruction(Transform);

	// A spawned zone is already initialized by the time OnConstruction runs (FinishSpawning comes after
	// InitInteractable), so only a placed one still needs its Details-panel fields pushed into Data.
	if (!Data.Owner)
	{
		Data.Params.Size = Radius;
		Data.Params.Color = Color;
		Data.Params.Attitude = AttitudeBitmask;
	}
	ApplyRadius();
	ApplyColor();
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoEffectZone::BeginPlay()
{
	// Hand-placed: no spawner calls InitInteractable, so initialize GAS here before Super inits default attributes.
	if (!Data.Owner)
	{
		Data.Owner = this;
		Data.Instigator = this;
		Data.TeamID = FGenericTeamId(static_cast<uint8>(Team));
		Data.Level = Level;
		Data.Params.Size = Radius;
		Data.Params.Color = Color;
		Data.Params.Attitude = AttitudeBitmask;
		Data.EffectDataArray = EffectDataArray;
		InitGas(Data.Owner);
	}

	Super::BeginPlay();
	ApplyRadius();
	ApplyColor();

	if (!GeoLib::IsServer(GetWorld()))
	{
		return;
	}

	CapsuleComponent->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnBeginOverlap);
	CapsuleComponent->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnEndOverlap);

	// A zone that lands on top of someone never gets a begin-overlap for them: the capsule grew to its real
	// radius back in InitInteractable, before these delegates existed. Catch whoever is already inside.
	TArray<AActor*> AlreadyInside;
	CapsuleComponent->GetOverlappingActors(AlreadyInside);
	for (AActor* Actor : AlreadyInside)
	{
		EnterZone(Actor);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoEffectZone::OnBeginOverlap(UPrimitiveComponent* /*OverlappedComponent*/, AActor* OtherActor,
									UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/,
									FHitResult const& /*SweepResult*/)
{
	EnterZone(OtherActor);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoEffectZone::EnterZone(AActor* OtherActor)
{
	if (OtherActor == this || !GeoASLib::GetGeoAscFromActor(OtherActor)
		|| !GeoASLib::IsTeamAttitudeAligned(this, OtherActor, Data.Params.Attitude))
	{
		return;
	}

	// Membership only — Tick does every apply. A downed actor stays tracked and simply stops receiving effects until
	// it can be damaged again, which is what lets someone revived inside the zone keep taking it.
	ActorsInZone.Add(OtherActor);
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

	UGeoAbilitySystemComponent* SourceASC = GeoASLib::GetGeoAscFromActor(Data.Owner);
	ensureMsgf(SourceASC, TEXT("AGeoEffectZone: missing ASC."));
	if (!SourceASC)
	{
		return;
	}

	TArray<TWeakObjectPtr<AActor>> Tracked;
	ActorsInZone.GetKeys(Tracked);
	for (TWeakObjectPtr<AActor> const& TrackedActor : Tracked)
	{
		ApplyZoneEffects(TrackedActor, SourceASC, DeltaSeconds);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoEffectZone::ApplyZoneEffects(TWeakObjectPtr<AActor> const& TrackedActor, UGeoAbilitySystemComponent* SourceASC,
									  float const DeltaSeconds)
{
	AActor* Actor = TrackedActor.Get();
	UGeoAbilitySystemComponent* TargetASC = GeoASLib::GetGeoAscFromActor(Actor);
	TArray<FActiveGameplayEffectHandle> const* Handles = ActorsInZone.Find(TrackedActor);
	if (!TargetASC || !Handles)
	{
		return;
	}

	bool const bApplyPersistent = Handles->IsEmpty();
	TArray<FActiveGameplayEffectHandle> AppliedHandles;
	for (TInstancedStruct<FEffectData> const& Entry : Data.EffectDataArray)
	{
		if (!IsValid(Actor) || !IsValid(TargetASC) || !Actor->CanBeDamaged())
		{
			break; // Do not continue if the actor is dead or destroyed during this loop.
		}

		if (FHealEffectData const* Heal = Entry.GetPtr<FHealEffectData>())
		{
			FHealEffectData Scaled = *Heal;
			Scaled.HealAmount = Heal->HealAmount.GetValueAtLevel(Data.Level) * DeltaSeconds;
			GeoASLib::ApplySingleEffectData(Scaled, SourceASC, TargetASC, Data.Level, Data.Seed, Data.AbilityTag);
		}
		else if (FDamageEffectData const* Damage = Entry.GetPtr<FDamageEffectData>())
		{
			FDamageEffectData Scaled = *Damage;
			Scaled.DamageAmount = Damage->DamageAmount.GetValueAtLevel(Data.Level) * DeltaSeconds;
			GeoASLib::ApplySingleEffectData(Scaled, SourceASC, TargetASC, Data.Level, Data.Seed, Data.AbilityTag);
		}
		else if (bApplyPersistent)
		{
			AppliedHandles.Add(
				GeoASLib::ApplySingleEffectData(Entry, SourceASC, TargetASC, Data.Level, Data.Seed, Data.AbilityTag));
		}
	}

	// Looked up again rather than held across the loop: a lethal entry runs its target's whole death and revive
	// from inside the apply above, and both ends of that re-enter the zone through the overlap delegates.
	if (TArray<FActiveGameplayEffectHandle>* Current = ActorsInZone.Find(TrackedActor);
		Current && !AppliedHandles.IsEmpty())
	{
		*Current = MoveTemp(AppliedHandles);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoEffectZone::OnRep_Data()
{
	ApplyRadius();
	ApplyColor();
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoEffectZone::ApplyRadius() const
{
	CapsuleComponent->SetCapsuleRadius(Data.Params.Size);
	CapsuleComponent->SetCapsuleHalfHeight(Data.Params.Size);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoEffectZone::ApplyColor() const
{
	FLinearColor const ZoneColor = Data.Params.Color.GetColor();
	for (UMeshComponent* const MeshComponent : GetVisualMeshComponents())
	{
		for (int32 MaterialIndex = 0; MaterialIndex < MeshComponent->GetNumMaterials(); ++MaterialIndex)
		{
			// Returns the existing instance when the slot already holds one, so repeated calls make no new material.
			// Null only for an empty material slot, which the engine already warns about.
			UMaterialInstanceDynamic* const Material =
				MeshComponent->CreateAndSetMaterialInstanceDynamic(MaterialIndex);
			if (!Material)
			{
				continue;
			}
			for (FName const& ParameterName : ColorParameterNames)
			{
				Material->SetVectorParameterValue(ParameterName, ZoneColor);
			}
		}
	}
}
