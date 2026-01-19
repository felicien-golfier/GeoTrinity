// Fill out your copyright notice in the Description page of Project Settings.

#include "GeoPlayerController.h"

#include "Camera/CameraActor.h"
#include "Engine/World.h"
#include "EnhancedInput/Public/EnhancedInputSubsystems.h"
#include "GameFramework/PlayerState.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Tool/GameplayLibrary.h"

AGeoPlayerController::AGeoPlayerController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SetShowMouseCursor(true);
}

void AGeoPlayerController::BeginPlay()
{
	Super::BeginPlay();
	SetViewTarget(UGameplayStatics::GetActorOfClass(GetWorld(), ACameraActor::StaticClass()));

	if (ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player))
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSystem =
				LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (!InputMapping.IsNull())
			{
				InputSystem->AddMappingContext(InputMapping.LoadSynchronous(), 0);
			}
		}
	}
}

void AGeoPlayerController::ServerSetAimYaw_Implementation(const float YawDegrees)
{
	if (IsValid(GetPawn()))
	{
		FRotator Rotation = GetPawn()->GetActorRotation();
		Rotation.Yaw = YawDegrees;
		GetPawn()->SetActorRotation(Rotation);
	}
}

AGeoPlayerController* AGeoPlayerController::GetLocalGeoPlayerController(const UWorld* World)
{
	for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		if (APlayerController* PlayerController = Iterator->Get())
		{
			if (PlayerController && PlayerController->IsLocalController()
				&& PlayerController->IsA(AGeoPlayerController::StaticClass()))
			{
				return CastChecked<AGeoPlayerController>(PlayerController);
			}
		}
	}
	return nullptr;
}