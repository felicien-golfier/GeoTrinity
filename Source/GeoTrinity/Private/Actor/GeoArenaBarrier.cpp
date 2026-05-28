// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/GeoArenaBarrier.h"

#include "Components/BoxComponent.h"
#include "Net/UnrealNetwork.h"

AGeoArenaBarrier::AGeoArenaBarrier()
{
	bReplicates = true;

	BlockingVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("BlockingVolume"));
	SetRootComponent(BlockingVolume);
	BlockingVolume->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BlockingVolume->SetCollisionResponseToAllChannels(ECR_Block);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(BlockingVolume);
}

void AGeoArenaBarrier::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGeoArenaBarrier, bIsClosed);
}

void AGeoArenaBarrier::SetClosed(bool bNewClosed)
{
	bIsClosed = bNewClosed;
	OnRep_bIsClosed();
}

void AGeoArenaBarrier::OnRep_bIsClosed()
{
	BlockingVolume->SetCollisionEnabled(bIsClosed ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	MeshComponent->SetVisibility(bIsClosed);
	OnBarrierStateChanged(bIsClosed);
}
