// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "World/GeoBackdropComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Tool/UGeoGameplayLibrary.h"

UGeoBackdropComponent::UGeoBackdropComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGeoBackdropComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!Active || GeoLib::IsDedicatedServer(this))
	{
		return;
	}
	if (!ensureMsgf(LayerMesh && !Layers.IsEmpty(),
					TEXT("UGeoBackdropComponent: no LayerMesh or no Layers — the camera renders no backdrop")))
	{
		return;
	}

	float const MeshSize = LayerMesh->GetBounds().BoxExtent.X * 2.f;
	for (int32 Index = 0; Index < Layers.Num(); ++Index)
	{
		UStaticMeshComponent* const Layer = NewObject<UStaticMeshComponent>(GetOwner());
		Layer->SetupAttachment(this);
		Layer->SetStaticMesh(LayerMesh);
		Layer->SetMaterial(0, Layers[Index]);
		Layer->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Layer->SetCastShadow(false);
		// The plane faces +Z, so pitching it up turns that normal onto the camera's -X: back at the camera.
		Layer->SetRelativeLocationAndRotation(FVector(FirstLayerDistance + Index * LayerSpacing, 0.f, 0.f),
											  FRotator(90.f, 0.f, 0.f));
		Layer->SetRelativeScale3D(FVector(LayerSize / MeshSize));
		Layer->RegisterComponent();
	}
}
