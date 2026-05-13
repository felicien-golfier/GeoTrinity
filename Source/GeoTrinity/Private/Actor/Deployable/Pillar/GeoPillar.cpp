// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Deployable/Pillar/GeoPillar.h"

#include "Components/CapsuleComponent.h"
#include "Net/UnrealNetwork.h"
#include "Tool/UGeoGameplayLibrary.h"

// ---------------------------------------------------------------------------------------------------------------------
AGeoPillar::AGeoPillar()
{
	bUseRegularDrain = true;
	CapsuleComponent->SetCapsuleSize(100.f, 100.f, false);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoPillar::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AGeoPillar, PillarData, COND_InitialOnly);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoPillar::InitInteractable(FInteractableActorData* Data)
{
	FDeployableData* InputData = static_cast<FDeployableData*>(Data);
	if (!ensureMsgf(InputData, TEXT("AGeoPillar: Data is not a FDeployableData!")))
	{
		return;
	}

	PillarData = *InputData;
	Super::InitInteractable(Data);
}

void AGeoPillar::RecallEffect(float Value)
{
	if (GeoLib::IsServer(GetWorld()))
	{
		Explode(Value);
	}
}
