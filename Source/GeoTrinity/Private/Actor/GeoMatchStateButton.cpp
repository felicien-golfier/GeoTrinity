// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/GeoMatchStateButton.h"

#include "Characters/PlayableCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameClasses/GeoGameMode.h"
#include "Tool/UGeoGameplayLibrary.h"

AGeoMatchStateButton::AGeoMatchStateButton()
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

void AGeoMatchStateButton::BeginPlay()
{
	Super::BeginPlay();
	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnBeginOverlap);
}

void AGeoMatchStateButton::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
										  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
										  FHitResult const& SweepResult)
{
	if (!GeoLib::IsServer(GetWorld()))
	{
		return;
	}

	if (!Cast<APlayableCharacter>(OtherActor))
	{
		return;
	}

	AGeoGameMode* GeoGameMode = GetWorld()->GetAuthGameMode<AGeoGameMode>();
	if (!ensureMsgf(GeoGameMode, TEXT("AGeoMatchStateButton %s has no AGeoGameMode"), *GetName()))
	{
		return;
	}

	switch (StateRequest)
	{
		case EGeoMatchStateRequest::StartMatch:
			GeoGameMode->StartMatch();
			break;
		case EGeoMatchStateRequest::WaitingToStart:
			GeoGameMode->RequestWaitingToStart();
			break;
		case EGeoMatchStateRequest::WaitingPostMatch:
			GeoGameMode->RequestWaitingPostMatch();
			break;
	}
}
