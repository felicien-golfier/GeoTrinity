// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Deployable/Mine/GeoMine.h"

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
	bExplodeAtRecall = true;
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoMine::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AGeoMine, MineData, COND_InitialOnly);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoMine::InitInteractable(FInteractableActorData* Data)
{
	FDeployableData* InputData = static_cast<FDeployableData*>(Data);
	if (!ensureMsgf(InputData, TEXT("AGeoMine: Data is not a FDeployableData!")))
	{
		return;
	}

	MineData = *InputData;
	Super::InitInteractable(Data);
}

void AGeoMine::ApplyExplodeEffect(float Value, UGeoAbilitySystemComponent* SourceASC, AActor* Actor,
								  UGeoAbilitySystemComponent* TargetASC)
{
	// Do not call super to not apply the Mine Life Cost effect.
	float const ScaledLifeSpent = MineData.Params.Value * Value;
	if (GeoASLib::IsTeamAttitudeAligned(MineData.Owner, Actor, TeamAttitudeMask::Hostile))
	{
		FDamageEffectData DamageEffect;
		DamageEffect.DamageAmount = FScalableFloat(ScaledLifeSpent);
		GeoASLib::ApplySingleEffectData(DamageEffect, SourceASC, TargetASC, MineData.Level, MineData.Seed,
										MineData.AbilityTag);
	}
	else if (GeoASLib::IsTeamAttitudeAligned(MineData.Owner, Actor, TeamAttitudeMask::FriendlyOrNeutral))
	{
		FShieldEffectData ShieldEffect;
		ShieldEffect.ShieldAmount = FScalableFloat(ScaledLifeSpent);
		GeoASLib::ApplySingleEffectData(ShieldEffect, SourceASC, TargetASC, MineData.Level, MineData.Seed,
										MineData.AbilityTag);
	}
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

	constexpr float RecallPowerModifier = 1.f; // When stepping on the mine, no modificator
	Recall(RecallPowerModifier);
}
