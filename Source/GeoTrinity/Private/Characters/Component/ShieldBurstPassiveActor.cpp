// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Characters/Component/ShieldBurstPassiveActor.h"

#include "AbilitySystem/Abilities/Square/GeoShieldBurstPassiveAbility.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"
#include "Tool/UGeoGameplayLibrary.h"

// ---------------------------------------------------------------------------------------------------------------------
AShieldBurstPassiveActor::AShieldBurstPassiveActor()
{
	SetReplicates(true);
	SetCanBeDamaged(false);
	PrimaryActorTick.bCanEverTick = false;
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);
}

// ---------------------------------------------------------------------------------------------------------------------
void AShieldBurstPassiveActor::BeginPlay()
{
	Super::BeginPlay();
	UGeoShieldBurstPassiveAbility const* ShieldBurstCDO =
		GeoASLib::GetAbilityCDO<UGeoShieldBurstPassiveAbility>(FGeoGameplayTags::Get().Ability_Spell_ShieldBurst);
	ensureMsgf(ShieldBurstCDO, TEXT("AShieldBurstPassiveActor: could not find ShieldBurst ability CDO"));
	if (ShieldBurstCDO)
	{
		ChargeTime = ShieldBurstCDO->ChargeTime;
	}

	AActor* OwnerActor = GetOwner();
	if (!ensureMsgf(IsValid(OwnerActor), TEXT("AShieldBurstPassiveActor: Owner is not valid")))
	{
		return;
	}

	SetActorLocation(OwnerActor->GetActorLocation() + FVector(ActorRelativeLocation, ArbitraryCharacterZ + 1.f));
	AttachToActor(OwnerActor, FAttachmentTransformRules::KeepRelativeTransform);

	// No material set on server
	if (!GeoLib::IsServer(GetWorld()))
	{
		InitializeMaterialInstances();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AShieldBurstPassiveActor::InitializeMaterialInstances()
{
	ACharacter const* Character = Cast<ACharacter>(GetOwner());

	if (!IsValid(Character))
	{
		ensureMsgf(IsValid(Character), TEXT("AShieldBurstPassiveActor: invalid Instigator"));
		return;
	}

	CharacterMaterialInstance = Character->GetMesh()->CreateAndSetMaterialInstanceDynamic(0);

	if (!IsValid(CharacterMaterialInstance))
	{
		ensureMsgf(IsValid(CharacterMaterialInstance), TEXT("AShieldBurstPassiveActor: invalid MaterialInstance"));
	}

	CharacterMaterialInstance->SetScalarParameterValue(GaugeScalarParamName, 0.f);
	CharacterMaterialInstance->SetScalarParameterValue(ChargeScalarParamName, 0.f);
}

// ---------------------------------------------------------------------------------------------------------------------
void AShieldBurstPassiveActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AShieldBurstPassiveActor, GaugeRatio);
}

// ---------------------------------------------------------------------------------------------------------------------
void AShieldBurstPassiveActor::SetGaugeRatio(float const NewRatio)
{
	GaugeRatio = NewRatio;
}

// ---------------------------------------------------------------------------------------------------------------------
void AShieldBurstPassiveActor::OnRep_GaugeRatio()
{
	OnGaugeRatioChanged(GaugeRatio);
	CharacterMaterialInstance->SetScalarParameterValue(GaugeScalarParamName, GaugeRatio);
	if (GaugeRatio >= 1.f)
	{
		StartChargeTime = GetWorld()->GetTimeSeconds();
		Charge();
	}
}

void AShieldBurstPassiveActor::Charge()
{
	constexpr float DischargeTime = .3f;

	float DeltaTime = GetWorld()->GetTimeSeconds() - StartChargeTime;

	if (DeltaTime < ChargeTime)
	{
		CharacterMaterialInstance->SetScalarParameterValue(ChargeScalarParamName, DeltaTime / ChargeTime);
		GetWorldTimerManager().SetTimerForNextTick(this, &AShieldBurstPassiveActor::Charge);
	}
	else if (DeltaTime < ChargeTime + DischargeTime)
	{
		CharacterMaterialInstance->SetScalarParameterValue(ChargeScalarParamName,
														   1 - (DeltaTime - ChargeTime) / DischargeTime);
		GetWorldTimerManager().SetTimerForNextTick(this, &AShieldBurstPassiveActor::Charge);
	}
	else
	{
		CharacterMaterialInstance->SetScalarParameterValue(ChargeScalarParamName, 0.f);
	}
}
