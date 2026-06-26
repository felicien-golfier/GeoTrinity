// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Characters/Component/ShieldBurstPassiveComponent.h"

#include "AbilitySystem/Abilities/Square/GeoShieldBurstPassiveAbility.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"
#include "Tool/UGeoGameplayLibrary.h"

// ---------------------------------------------------------------------------------------------------------------------
UShieldBurstPassiveComponent::UShieldBurstPassiveComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

// ---------------------------------------------------------------------------------------------------------------------
void UShieldBurstPassiveComponent::BeginPlay()
{
	Super::BeginPlay();
	UGeoShieldBurstPassiveAbility const* ShieldBurstCDO =
		GeoASLib::GetAbilityCDO<UGeoShieldBurstPassiveAbility>(FGeoGameplayTags::Get().Ability_Spell_ShieldBurst);
	ensureMsgf(ShieldBurstCDO, TEXT("UShieldBurstPassiveComponent: could not find ShieldBurst ability CDO"));
	if (ShieldBurstCDO)
	{
		ChargeTime = ShieldBurstCDO->ChargeTime;
	}

	AActor* OwnerActor = GetOwner();
	if (!ensureMsgf(IsValid(OwnerActor), TEXT("UShieldBurstPassiveComponent: Owner is not valid")))
	{
		return;
	}

	// Material is cosmetic: set it on every rendering machine including the listen-server host. Only the dedicated
	// server (no viewport) skips it.
	if (!GeoLib::IsDedicatedServer(GetWorld()))
	{
		InitializeMaterialInstances();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UShieldBurstPassiveComponent::InitializeMaterialInstances()
{
	ACharacter const* Character = Cast<ACharacter>(GetOwner());

	if (!IsValid(Character))
	{
		ensureMsgf(IsValid(Character), TEXT("UShieldBurstPassiveComponent: invalid Instigator"));
		return;
	}

	CharacterMaterialInstance = Character->GetMesh()->CreateAndSetMaterialInstanceDynamic(0);

	if (!IsValid(CharacterMaterialInstance))
	{
		ensureMsgf(IsValid(CharacterMaterialInstance), TEXT("UShieldBurstPassiveComponent: invalid MaterialInstance"));
	}

	CharacterMaterialInstance->SetScalarParameterValue(GaugeScalarParamName, 0.f);
	CharacterMaterialInstance->SetScalarParameterValue(ChargeScalarParamName, 0.f);
}

// ---------------------------------------------------------------------------------------------------------------------
void UShieldBurstPassiveComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UShieldBurstPassiveComponent, GaugeRatio);
}

// ---------------------------------------------------------------------------------------------------------------------
void UShieldBurstPassiveComponent::SetGaugeRatio(float const NewRatio)
{
	GaugeRatio = NewRatio;
	OnRep_GaugeRatio(); // For Host.
}

// ---------------------------------------------------------------------------------------------------------------------
void UShieldBurstPassiveComponent::OnRep_GaugeRatio()
{
	OnGaugeRatioChanged(GaugeRatio);

	// Do not change the value when the Discharge is not ended.
	float DeltaTime = GetWorld()->GetTimeSeconds() - StartChargeTime;
	if (DeltaTime > ChargeTime + DischargeTime)
	{
		CharacterMaterialInstance->SetScalarParameterValue(GaugeScalarParamName, GaugeRatio);
	}

	if (GaugeRatio >= 1.f)
	{
		StartChargeTime = GetWorld()->GetTimeSeconds();
		Charge();
	}
}

void UShieldBurstPassiveComponent::Charge()
{

	float DeltaTime = GetWorld()->GetTimeSeconds() - StartChargeTime;

	if (DeltaTime < ChargeTime)
	{
		CharacterMaterialInstance->SetScalarParameterValue(ChargeScalarParamName, DeltaTime / ChargeTime);
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UShieldBurstPassiveComponent::Charge);
	}
	else if (DeltaTime < ChargeTime + DischargeTime)
	{
		CharacterMaterialInstance->SetScalarParameterValue(ChargeScalarParamName,
														   1 - (DeltaTime - ChargeTime) / DischargeTime);
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UShieldBurstPassiveComponent::Charge);
	}
	else
	{
		CharacterMaterialInstance->SetScalarParameterValue(GaugeScalarParamName, 0.f);
		CharacterMaterialInstance->SetScalarParameterValue(ChargeScalarParamName, 0.f);
	}
}
