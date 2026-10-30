// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Characters/Component/FX/GeoBeamVFXComponent.h"

#include "Net/UnrealNetwork.h"
#include "Tool/UGeoGameplayLibrary.h"
#include "World/ZoneLayer/GeoZoneVisualComponent.h"

// ---------------------------------------------------------------------------------------------------------------------
UGeoBeamVFXComponent::UGeoBeamVFXComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoBeamVFXComponent::BeginPlay()
{
	Super::BeginPlay();

	// Cosmetic: drawn on every rendering machine including the listen-server host; only the dedicated server skips it.
	if (GeoLib::IsDedicatedServer(GetWorld()))
	{
		return;
	}

	ZoneVisual = NewObject<UGeoZoneVisualComponent>(GetOwner());
	ZoneVisual->Shape.Shape = EGeoZoneShape::Beam;
	// BeamState and ColorMask may already hold live values: the component can arrive via replication after the
	// ability fired.
	ApplyBeamState();
	ZoneVisual->RegisterComponent();
	ZoneVisual->AttachToComponent(GetOwner()->GetRootComponent(),
								  FAttachmentTransformRules::SnapToTargetNotIncludingScale);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoBeamVFXComponent::EndPlay(EEndPlayReason::Type const EndPlayReason)
{
	if (ZoneVisual)
	{
		ZoneVisual->DestroyComponent();
		ZoneVisual = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoBeamVFXComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGeoBeamVFXComponent, BeamState);
	// Not COND_InitialOnly: the component is added dynamically (OnGiveAbility), after the owning actor's channel
	// already sent its initial bunch — InitialOnly properties would then never reach existing clients.
	DOREPLIFETIME(UGeoBeamVFXComponent, ColorMask);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoBeamVFXComponent::SetBeamState(bool const bActive, float const HalfWidth, float const Length)
{
	BeamState = {bActive, HalfWidth, Length};
	// Apply locally too: the listen-server host renders but never receives OnRep for its own writes.
	ApplyBeamState();
}

void UGeoBeamVFXComponent::SetBeamColors(FGeoColorParam const& Color, TArray<FGeoColorParam> const& SecondaryColors)
{
	ColorMask = GeoZoneLayer::MakeColorMask(Color, SecondaryColors);
	ApplyBeamState();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoBeamVFXComponent::OnRep_BeamState() const
{
	ApplyBeamState();
}

void UGeoBeamVFXComponent::OnRep_ColorMask() const
{
	ApplyBeamState();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoBeamVFXComponent::ApplyBeamState() const
{
	// Null on the dedicated server (never created) — nothing to draw there.
	if (ZoneVisual)
	{
		ZoneVisual->Shape.Length = BeamState.Length;
		ZoneVisual->Shape.HalfWidth = BeamState.HalfWidth;
		ZoneVisual->ColorMask = ColorMask;
		ZoneVisual->SetZoneVisible(BeamState.bActive);
	}
}
