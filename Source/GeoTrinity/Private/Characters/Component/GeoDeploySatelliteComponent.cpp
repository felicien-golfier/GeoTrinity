// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Characters/Component/GeoDeploySatelliteComponent.h"

#include "AbilitySystem/Abilities/Common/GeoDeployAbility.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Tool/UGeoGameplayLibrary.h"

// An exponential interp is within 1% of its target after five time constants, so this turns TravelTime into a speed.
static constexpr float TimeConstantsToSettle = 5.f;

// ---------------------------------------------------------------------------------------------------------------------
UGeoDeploySatelliteComponent::UGeoDeploySatelliteComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoDeploySatelliteComponent::TickComponent(float const DeltaTime, ELevelTick const TickType,
												 FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!GeoLib::IsLocalPlayerAvatar(GetOwner()))
	{
		return;
	}

	int32 const DesiredCount = GetDesiredSatelliteCount();
	while (Satellites.Num() > DesiredCount)
	{
		Satellites.Pop()->DestroyComponent();
	}
	for (int32 Missing = DesiredCount - Satellites.Num(); Missing > 0; --Missing)
	{
		AddSatellite();
	}

	OrbitAngle = FMath::Fmod(OrbitAngle + Params.OrbitSpeed * DeltaTime, 360.f);
	for (int32 Index = 0; Index < Satellites.Num(); ++Index)
	{
		// One interp covers both moves: it flies a fresh satellite out from the character, and slides the survivors
		// into their new spacing when one leaves the ring.
		Satellites[Index]->SetRelativeLocation(FMath::VInterpTo(Satellites[Index]->GetRelativeLocation(),
															   GetSlotLocation(Index, Satellites.Num()), DeltaTime,
															   TimeConstantsToSettle / Params.TravelTime));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoDeploySatelliteComponent::SetParams(FSatelliteParams const& NewParams)
{
	Params = NewParams;

	for (UPrimitiveComponent* Satellite : Satellites)
	{
		Satellite->DestroyComponent();
	}
	Satellites.Empty();
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoDeploySatelliteComponent::LaunchSatellite(FVector& OutLaunchLocation)
{
	if (Satellites.IsEmpty())
	{
		return false;
	}

	// The one in front reads as the source of the shot; any other would fire through the character.
	FVector const Forward = GetOwner()->GetActorForwardVector();
	FVector const RingCentre = GetComponentLocation();
	int32 LaunchedIndex = 0;
	float BestDot = TNumericLimits<float>::Lowest();
	for (int32 Index = 0; Index < Satellites.Num(); ++Index)
	{
		float const Dot = FVector::DotProduct(Satellites[Index]->GetComponentLocation() - RingCentre, Forward);
		if (Dot > BestDot)
		{
			BestDot = Dot;
			LaunchedIndex = Index;
		}
	}

	UPrimitiveComponent* Launched = Satellites[LaunchedIndex];
	OutLaunchLocation = Launched->GetComponentLocation();
	Satellites.RemoveAt(LaunchedIndex);
	Launched->DestroyComponent();
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
int32 UGeoDeploySatelliteComponent::GetDesiredSatelliteCount() const
{
	UGeoAbilitySystemComponent* ASC = GeoASLib::GetGeoAscFromActor(GetOwner());
	if (!ASC)
	{
		return 0;
	}

	for (FGameplayAbilitySpec const& Spec : ASC->GetActivatableAbilities())
	{
		// Spec.Ability is the CDO and holds no stack state; only the per-actor instance can answer.
		if (UGeoDeployAbility const* DeployAbility = Cast<UGeoDeployAbility>(Spec.GetPrimaryInstance()))
		{
			// The charge is spent on the press, but its satellite only leaves the ring once the shot launches it.
			return DeployAbility->GetCurrentStacks() + (Spec.IsActive() ? 1 : 0);
		}
	}

	return 0;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoDeploySatelliteComponent::AddSatellite()
{
	if (!ensureMsgf(Params.Mesh || Params.System,
					TEXT("%s: its class data sets neither a satellite Mesh nor System, deploy charges show nothing."),
					*GetNameSafe(GetOwner())))
	{
		return;
	}

	// A satellite is one component or the other, never a mesh and a system on the same slot.
	UPrimitiveComponent* Satellite;
	if (Params.System)
	{
		UNiagaraComponent* NiagaraSatellite = NewObject<UNiagaraComponent>(GetOwner());
		NiagaraSatellite->SetAsset(Params.System);
		Satellite = NiagaraSatellite;
	}
	else
	{
		UStaticMeshComponent* MeshSatellite = NewObject<UStaticMeshComponent>(GetOwner());
		MeshSatellite->SetStaticMesh(Params.Mesh);
		Satellite = MeshSatellite;
	}

	Satellite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Satellite->SetCastShadow(false);
	Satellite->SetRelativeScale3D(FVector(Params.Scale));
	// Attached before registering, so it is already on the character when it first ticks: a Niagara satellite
	// auto-activates on registration and would spawn its first particles at the world origin otherwise. Relative zero
	// puts every satellite at the character's centre; the tick flies it out to its slot.
	Satellite->SetupAttachment(this);
	Satellite->RegisterComponent();
	Satellites.Add(Satellite);
}

// ---------------------------------------------------------------------------------------------------------------------
FVector UGeoDeploySatelliteComponent::GetSlotLocation(int32 const Index, int32 const Count) const
{
	float const SlotAngle = FMath::DegreesToRadians(OrbitAngle + Index * 360.f / Count);
	return FVector(FMath::Cos(SlotAngle), FMath::Sin(SlotAngle), 0.f) * Params.OrbitRadius;
}
