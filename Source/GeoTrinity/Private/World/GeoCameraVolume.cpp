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

	BoundsBox = CreateDefaultSubobject<UBoxComponent>(TEXT("BoundsBox"));
	BoundsBox->SetupAttachment(TriggerBox);
	BoundsBox->SetBoxExtent(FVector(1000.f));
	BoundsBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BoundsBox->ShapeColor = FColor::Cyan;
}

FBox2D AGeoCameraVolume::GetViewBounds() const
{
	if (!bLimitView)
	{
		return FBox2D(ForceInit);
	}

	FBox const Box = BoundsBox->Bounds.GetBox();
	return FBox2D(FVector2D(Box.Min), FVector2D(Box.Max));
}

void AGeoCameraVolume::OnConstruction(FTransform const& Transform)
{
	Super::OnConstruction(Transform);
	BoundsBox->SetVisibility(bLimitView);
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
