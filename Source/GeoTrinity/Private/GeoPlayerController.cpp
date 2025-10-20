// Fill out your copyright notice in the Description page of Project Settings.

#include "GeoPlayerController.h"

#include "Camera/CameraActor.h"
#include "EnhancedInput/Public/EnhancedInputSubsystems.h"
#include "Kismet/GameplayStatics.h"

AGeoPlayerController::AGeoPlayerController( const FObjectInitializer& ObjectInitializer ) : Super( ObjectInitializer )
{
}

void AGeoPlayerController::BeginPlay()
{
	Super::BeginPlay();
	SetViewTarget( UGameplayStatics::GetActorOfClass( GetWorld(), ACameraActor::StaticClass() ) );

	if ( ULocalPlayer* LocalPlayer = Cast< ULocalPlayer >( Player ) ) {
		if ( UEnhancedInputLocalPlayerSubsystem* InputSystem = LocalPlayer->GetSubsystem< UEnhancedInputLocalPlayerSubsystem >() ) {
			if ( !InputMapping.IsNull() ) {
				InputSystem->AddMappingContext( InputMapping.LoadSynchronous(), 0 );
			}
		}
	}
}

void AGeoPlayerController::OnPossess( APawn* APawn )
{
	Super::OnPossess( APawn );
}