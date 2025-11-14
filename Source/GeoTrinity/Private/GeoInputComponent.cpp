// Fill out your copyright notice in the Description page of Project Settings.

#include "GeoInputComponent.h"

#include "EnhancedInputComponent.h"
#include "GeoPawn.h"
#include "GeoPlayerController.h"
#include "VisualLogger/VisualLogger.h"

UGeoInputComponent::UGeoInputComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGeoInputComponent::BindInput(UInputComponent* PlayerInputComponent)
{
	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &UGeoInputComponent::MoveFromInput);
}

void UGeoInputComponent::MoveFromInput(const FInputActionInstance& Instance)
{
	if (GetNetMode() == NM_Standalone)
	{
		GetGeoPawn()->AddMovementInput(FVector(Instance.GetValue().Get<FVector2D>(), 0.f));
	}
	else if (AGeoPlayerController* PlayerController = Cast<AGeoPlayerController>(GetGeoPawn()->GetController()))
	{
		PlayerController->ServerMove(Instance.GetValue().Get<FVector2D>());
	}
}

AGeoPawn* UGeoInputComponent::GetGeoPawn() const
{
	return CastChecked<AGeoPawn>(GetOuter());
}
