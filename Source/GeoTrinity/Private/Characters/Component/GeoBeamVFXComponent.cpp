// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Characters/Component/GeoBeamVFXComponent.h"

#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "Tool/UGeoGameplayLibrary.h"

// ---------------------------------------------------------------------------------------------------------------------
UGeoBeamVFXComponent::UGeoBeamVFXComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UGeoBeamVFXComponent::CreateNiagaraComponent()
{
	if (!IsValid(NiagaraComponent) && BeamSystem)
	{
		NiagaraComponent = NewObject<UNiagaraComponent>(GetOwner());
		NiagaraComponent->SetAutoActivate(false);
		NiagaraComponent->SetAsset(BeamSystem);
		NiagaraComponent->RegisterComponent();
		NiagaraComponent->AttachToComponent(GetOwner()->GetRootComponent(),
											FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}
}
// ---------------------------------------------------------------------------------------------------------------------
void UGeoBeamVFXComponent::BeginPlay()
{
	Super::BeginPlay();

	// VFX is cosmetic: spawn on every rendering machine including the listen-server host; only the dedicated
	// server (no viewport) skips it.
	if (GeoLib::IsDedicatedServer(GetWorld()))
	{
		return;
	}

	if (BeamSystem)
	{
		CreateNiagaraComponent();
		// BeamState may already hold live values when this component arrives via replication after the ability fired.
		ApplyBeamState();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoBeamVFXComponent::EndPlay(EEndPlayReason::Type const EndPlayReason)
{
	if (NiagaraComponent)
	{
		NiagaraComponent->DestroyComponent();
		NiagaraComponent = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoBeamVFXComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGeoBeamVFXComponent, BeamState);
	DOREPLIFETIME_CONDITION(UGeoBeamVFXComponent, BeamSystem, COND_InitialOnly);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoBeamVFXComponent::SetBeamState(bool const bActive, float const Width, float const Length)
{
	BeamState = {bActive, Width, Length};
	// Apply locally too: the listen-server host renders but never receives OnRep for its own writes.
	ApplyBeamState();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoBeamVFXComponent::OnRep_BeamState() const
{
	ApplyBeamState();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoBeamVFXComponent::ApplyBeamState() const
{
	// Null on the dedicated server (never spawned) — nothing to drive there.
	if (!NiagaraComponent)
	{
		return;
	}

	if (BeamState.bActive)
	{
		if (!NiagaraComponent->IsActive())
		{
			NiagaraComponent->SetActive(true);
		}

		NiagaraComponent->SetVariableFloat(HalfWidthParamName, BeamState.Width);
		NiagaraComponent->SetVariableFloat(LengthParamName, BeamState.Length);
	}
	else if (NiagaraComponent->IsActive())
	{
		NiagaraComponent->SetActive(false, true);
	}
}
