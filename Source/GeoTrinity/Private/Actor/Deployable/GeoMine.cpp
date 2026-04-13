// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Deployable/GeoMine.h"

#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Components/CapsuleComponent.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Tool/Team.h"

// ---------------------------------------------------------------------------------------------------------------------
AGeoMine::AGeoMine()
{
	bUseRegularDrain = false;
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoMine::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AGeoMine, MineData, COND_InitialOnly);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoMine::InitInteractableData(FInteractableActorData* Data)
{
	FDeployableData* InputData = static_cast<FDeployableData*>(Data);
	if (!ensureMsgf(InputData, TEXT("AGeoMine: Data is not a FDeployableData!")))
	{
		return;
	}

	MineData = *InputData;
	bIsRecalling = false;
	Super::InitInteractableData(Data);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoMine::Recall(float Value)
{
	if (bExpired || bIsRecalling)
	{
		return;
	}

	bIsRecalling = true;

	UGeoAbilitySystemComponent* SourceASC = GeoASLib::GetGeoAscFromActor(MineData.CharacterOwner);
	if (!ensureMsgf(SourceASC, TEXT("AGeoMine: no ASC on CharacterOwner")))
	{
		Super::Recall(Value);
		return;
	}

	TArray<AActor*> OverlappingActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes = {UEngineTypes::ConvertToObjectType(ECC_Pawn),
														 UEngineTypes::ConvertToObjectType(ECC_GeoCharacter)};
	UKismetSystemLibrary::SphereOverlapActors(this, GetActorLocation(), MineData.Params.Size, ObjectTypes, nullptr, {},
											  OverlappingActors);

	float const ScaledLifeSpent = MineData.Params.Value * Value;

	for (AActor* Actor : OverlappingActors)
	{
		if (!IsValid(Actor) || Actor == MineData.CharacterOwner)
		{
			continue;
		}

		if (GeoASLib::IsTeamAttitudeAligned(MineData.CharacterOwner, Actor,
											static_cast<int32>(ETeamAttitudeBitflag::Hostile)))
		{
			UGeoAbilitySystemComponent* TargetASC = GeoASLib::GetGeoAscFromActor(Actor);
			if (TargetASC)
			{
				FDamageEffectData DamageEffect;
				DamageEffect.DamageAmount = FScalableFloat(ScaledLifeSpent);
				GeoASLib::ApplySingleEffectData(DamageEffect, SourceASC, TargetASC, MineData.Level, MineData.Seed);
			}
		}
		else if (GeoASLib::IsTeamAttitudeAligned(MineData.CharacterOwner, Actor,
												 static_cast<int32>(ETeamAttitudeBitflag::Friendly)))
		{
			UGeoAbilitySystemComponent* TargetASC = GeoASLib::GetGeoAscFromActor(Actor);
			if (TargetASC)
			{
				FShieldEffectData ShieldEffect;
				ShieldEffect.ShieldAmount = FScalableFloat(ScaledLifeSpent);
				GeoASLib::ApplySingleEffectData(ShieldEffect, SourceASC, TargetASC, MineData.Level, MineData.Seed);
			}
		}
	}

	Super::Recall(Value);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoMine::BeginPlay()
{
	Super::BeginPlay();

	CapsuleComponent->OnComponentBeginOverlap.AddDynamic(this, &AGeoMine::OnCapsuleBeginOverlap);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoMine::OnCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
									 UPrimitiveComponent* OtherOverlappedComponent, int32 OtherBodyIndex,
									 bool bFromSweep, FHitResult const& SweepResult)
{
	if (!OtherActor->CanBeDamaged())
	{
		return;
	}

	Recall(1.f);
}
