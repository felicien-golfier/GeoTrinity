// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/GeoClassChangeTrigger.h"

#include "Characters/PlayableCharacter.h"
#include "Tool/UGeoGameplayLibrary.h"

AGeoClassChangeTrigger::AGeoClassChangeTrigger()
{
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);
	MeshComponent->CastShadow = false;
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Overlap);
}

void AGeoClassChangeTrigger::BeginPlay()
{
	Super::BeginPlay();
	MeshComponent->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnBeginOverlap);
}

void AGeoClassChangeTrigger::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
											UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
											FHitResult const& SweepResult)
{
	if (!GeoLib::IsServer(GetWorld()))
	{
		return;
	}

	if (TargetClass == EPlayerClass::None)
	{
		ensureMsgf(TargetClass != EPlayerClass::None, TEXT("AGeoClassChangeTrigger %s has TargetClass = None"),
				   *GetName());
		return;
	}

	APlayableCharacter* PlayableCharacter = Cast<APlayableCharacter>(OtherActor);
	if (!PlayableCharacter)
	{
		return;
	}

	if (PlayableCharacter->GetPlayerClass() == TargetClass)
	{
		return;
	}

	PlayableCharacter->ChangeClass(TargetClass);
}
