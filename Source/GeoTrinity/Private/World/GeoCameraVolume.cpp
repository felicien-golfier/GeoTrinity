// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "World/GeoCameraVolume.h"

#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Tool/UGeoGameplayLibrary.h"
#include "World/GeoGameCamera.h"

AGeoCameraVolume::AGeoCameraVolume()
{
	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	SetRootComponent(TriggerBox);
	TriggerBox->SetBoxExtent(FVector(500.f));
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Overlap);
	TriggerBox->SetGenerateOverlapEvents(true);
}

void AGeoCameraVolume::BeginPlay()
{
	Super::BeginPlay();
	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnBeginOverlap);
	TriggerBox->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnEndOverlap);
}

void AGeoCameraVolume::OnBeginOverlap(UPrimitiveComponent* /*OverlappedComponent*/, AActor* OtherActor,
									  UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/,
									  FHitResult const& /*SweepResult*/)
{
	if (!GeoLib::IsLocalPlayerAvatar(OtherActor))
	{
		return;
	}
	if (AGeoGameCamera* Camera =
			Cast<AGeoGameCamera>(UGameplayStatics::GetActorOfClass(this, AGeoGameCamera::StaticClass())))
	{
		Camera->EnterVolume(this);
	}
}

void AGeoCameraVolume::OnEndOverlap(UPrimitiveComponent* /*OverlappedComponent*/, AActor* OtherActor,
									UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/)
{
	if (!GeoLib::IsLocalPlayerAvatar(OtherActor))
	{
		return;
	}
	if (AGeoGameCamera* Camera =
			Cast<AGeoGameCamera>(UGameplayStatics::GetActorOfClass(this, AGeoGameCamera::StaticClass())))
	{
		Camera->ExitVolume(this);
	}
}
