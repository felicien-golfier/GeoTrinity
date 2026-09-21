// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Deployable/Wall/GeoWall.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

// ---------------------------------------------------------------------------------------------------------------------
AGeoWall::AGeoWall(FObjectInitializer const& ObjectInitializer) : Super(ObjectInitializer)
{
	bUseRegularDrain = true;
	bExplodeAtRecall = false;
	// Gameplay collision moves from the root capsule onto the mesh: the mesh carries the GeoShape profile so
	// projectiles hit its exact shape, while the capsule keeps no collision (still the root for transform/replication).
	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(GetRootComponent());
	MeshComponent->SetCollisionProfileName(TEXT("GeoShape"));
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoWall::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AGeoWall, WallData, COND_InitialOnly);
}

// ---------------------------------------------------------------------------------------------------------------------
bool AGeoWall::DoesOverlapShape(FVector2D const Location, FVector2D const Center, float const Radius) const
{
	// Location is where the caller holds the wall, so the circle moves by what it is off the wall's own place.
	FVector2D const RelativeLocation = Center + FVector2D(GetActorLocation()) - Location;
	FVector const RelativeLocation3D(RelativeLocation, MeshComponent->Bounds.Origin.Z);
	return MeshComponent->OverlapComponent(RelativeLocation3D, FQuat::Identity, FCollisionShape::MakeSphere(Radius));
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoWall::InitInteractable(FInteractableActorData* Data)
{
	FDeployableData* InputData = static_cast<FDeployableData*>(Data);
	if (!ensureMsgf(InputData, TEXT("AGeoWall: Data is not a FDeployableData!")))
	{
		return;
	}

	WallData = *InputData;
	Super::InitInteractable(Data);
}
