// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/GeoFloorButton.h"

#include "Characters/PlayableCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "Tool/UGeoGameplayLibrary.h"

AGeoFloorButton::AGeoFloorButton()
{
	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	SetRootComponent(TriggerBox);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Overlap);

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(TriggerBox);
	// Lay the text flat on the XY plane facing up toward the top-down camera.
	Label->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetVerticalAlignment(EVRTA_TextCenter);
}

void AGeoFloorButton::BeginPlay()
{
	Super::BeginPlay();
	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnBeginOverlap);
}

void AGeoFloorButton::OnBeginOverlap(UPrimitiveComponent* /*OverlappedComponent*/, AActor* OtherActor,
									 UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/,
									 FHitResult const& /*SweepResult*/)
{
	if (GeoLib::IsServer(GetWorld()) && Cast<APlayableCharacter>(OtherActor))
	{
		Press();
	}
}
