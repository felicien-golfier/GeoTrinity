// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Deployable/Pillar/GeoPillar.h"

#include "Components/CapsuleComponent.h"
#include "Net/UnrealNetwork.h"

// ---------------------------------------------------------------------------------------------------------------------
AGeoPillar::AGeoPillar()
{
	bUseRegularDrain = false;
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

// ---------------------------------------------------------------------------------------------------------------------
void AGeoPillar::OnHealthChanged_Implementation(float NewValue)
{
	// Let base handle blink/expiry from drain; pillar has no drain so this only fires from damage.
	Super::OnHealthChanged_Implementation(NewValue);

	if (NewValue <= 0.f && !IsExpired())
	{
		OnPillarDestroyed();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoPillar::OnPillarDestroyed_Implementation()
{
	// Base implementation intentionally empty — BP overrides for VFX/SFX.
}
