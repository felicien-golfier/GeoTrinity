// Fill out your copyright notice in the Description page of Project Settings.

#include "GeoInputComponent.h"

#include "EnhancedInputComponent.h"
#include "GeoCharacter.h"
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

	if (LookAction)
	{
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this,
			&UGeoInputComponent::LookFromInput);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Completed, this,
			&UGeoInputComponent::LookFromInput);
	}
}

void UGeoInputComponent::MoveFromInput(const FInputActionInstance& Instance)
{
	GetGeoCharacter()->AddMovementInput(FVector(Instance.GetValue().Get<FVector2D>(), 0.f));
}

void UGeoInputComponent::LookFromInput(const FInputActionInstance& Instance)
{
	// Cache latest right stick / look vector
	if (Instance.GetTriggerEvent() == ETriggerEvent::Completed)
	{
		LastLookInput = FVector2D::ZeroVector;
	}
	else
	{
		LastLookInput = Instance.GetValue().Get<FVector2D>();
	}
}

AGeoCharacter* UGeoInputComponent::GetGeoCharacter() const
{
	return CastChecked<AGeoCharacter>(GetOuter());
}
