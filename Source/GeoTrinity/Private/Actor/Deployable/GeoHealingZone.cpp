// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Deployable/GeoHealingZone.h"

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Tool/UGeoGameplayLibrary.h"

AGeoHealingZone::AGeoHealingZone()
{
	bUseRegularDrain = false;
	SetCanBeDamaged(false);
}

void AGeoHealingZone::InitDrain()
{
	ensureMsgf(Data.Params.LifeDrainMaxDuration > 0.f,
			   TEXT("HealingZone cannot have no duration. please fill your data correctly"));

	if (!GeoLib::IsServer(GetWorld()))
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	float const MaxHealth = ASC->GetNumericAttribute(UGeoAttributeSetBase::GetMaxHealthAttribute());
	ensureMsgf(MaxHealth > 0.f, TEXT("AGeoHealingZone: MaxHealth is 0 — DefaultAttributes may not be applied."));
	if (MaxHealth <= 0.f)
	{
		return;
	}

	DrainMagnitudePerSecond = MaxHealth / Data.Params.LifeDrainMaxDuration;
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoHealingZone::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AGeoHealingZone, Data, COND_InitialOnly);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoHealingZone::InitInteractableData(FInteractableActorData* InputData)
{
	FDeployableData* DeployableData = static_cast<FDeployableData*>(InputData);
	ensureMsgf(DeployableData, TEXT("AGeoHealingZone: Data is not FHealingZoneData!"));
	if (!DeployableData)
	{
		return;
	}
	Data = *DeployableData;

	CapsuleComponent->SetCapsuleHalfHeight(Data.Params.Size);
	CapsuleComponent->SetCapsuleRadius(Data.Params.Size);
	CapsuleComponent->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnBeginOverlap);
	CapsuleComponent->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnEndOverlap);
	Super::InitInteractableData(InputData);
}

// ---------------------------------------------------------------------------------------------------------------------
float AGeoHealingZone::GetDurationPercent() const
{
	UAbilitySystemComponent const* ASC = GetAbilitySystemComponent();
	float const MaxHealth = ASC->GetNumericAttribute(UGeoAttributeSetBase::GetMaxHealthAttribute());
	if (MaxHealth <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(ASC->GetNumericAttribute(UGeoAttributeSetBase::GetHealthAttribute()) / MaxHealth, 0.f, 1.f);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoHealingZone::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
									 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
									 FHitResult const& SweepResult)
{
	if (!UGeoAbilitySystemLibrary::GetGeoAscFromActor(OtherActor) || OtherActor == this)
	{
		return;
	}
	ActorsInZone.Add(OtherActor);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoHealingZone::OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
								   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ActorsInZone.Remove(OtherActor);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoHealingZone::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!GeoLib::IsServer(GetWorld()))
	{
		return;
	}
	UAbilitySystemComponent* OwnerASC = GeoASLib::GetGeoAscFromActor(GetData()->CharacterOwner);

	IGenericTeamAgentInterface const* ZoneTeamAgent = Cast<IGenericTeamAgentInterface>(this);

	int HealedNum = 0;

	for (TWeakObjectPtr<AActor> const WeakActor : ActorsInZone)
	{
		AActor* Actor = WeakActor.Get();
		if (!IsValid(Actor))
		{
			continue;
		}
		if (ZoneTeamAgent->GetTeamAttitudeTowards(*Actor) == ETeamAttitude::Hostile)
		{
			continue;
		}
		UGeoAbilitySystemComponent* TargetASC = UGeoAbilitySystemLibrary::GetGeoAscFromActor(Actor);
		if (!TargetASC || !TargetASC->GetAvatarActor()->CanBeDamaged())
		{
			continue;
		}

		if (TargetASC->GetNumericAttribute(UGeoAttributeSetBase::GetHealthAttribute())
			>= TargetASC->GetNumericAttribute(UGeoAttributeSetBase::GetMaxHealthAttribute()))
		{
			continue; // Do not heal, neither count allies full life.
		}

		// Also add the array, but that not the heal. This is to let game design decide if they want to add something
		UGeoAbilitySystemLibrary::ApplyEffectFromEffectData(Data.EffectDataArray, OwnerASC, TargetASC, Data.Level,
															Data.Seed);
		FHealEffectData HealEffectData = FHealEffectData();
		HealEffectData.HealAmount = DrainMagnitudePerSecond * DeltaSeconds;
		UGeoAbilitySystemLibrary::ApplySingleEffectData(HealEffectData, OwnerASC, TargetASC, Data.Level, Data.Seed);
		++HealedNum;
	}

	if (HealedNum > 0)
	{
		FDamageEffectData DrainEffectData = FDamageEffectData();
		DrainEffectData.DamageAmount = DrainMagnitudePerSecond * DeltaSeconds * HealedNum;
		UGeoAbilitySystemLibrary::ApplySingleEffectData(DrainEffectData, OwnerASC, GetAbilitySystemComponent(),
														Data.Level, Data.Seed);
	}
}

void AGeoHealingZone::OnRep_Data() const
{
	CapsuleComponent->SetCapsuleHalfHeight(Data.Params.Size);
	CapsuleComponent->SetCapsuleRadius(Data.Params.Size);
}
