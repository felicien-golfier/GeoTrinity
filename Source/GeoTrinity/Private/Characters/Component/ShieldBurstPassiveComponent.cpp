// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Characters/Component/ShieldBurstPassiveComponent.h"

#include "AbilitySystem/Abilities/Square/GeoShieldBurstPassiveAbility.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Net/UnrealNetwork.h"

// ---------------------------------------------------------------------------------------------------------------------
UShieldBurstPassiveComponent::UShieldBurstPassiveComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
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
}

// ---------------------------------------------------------------------------------------------------------------------
void UShieldBurstPassiveComponent::OnRep_GaugeRatio()
{
	OnGaugeRatioChanged(GaugeRatio);
}
