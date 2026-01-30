// Fill out your copyright notice in the Description page of Project Settings.

#include "Input/GeoInputComponent.h"

#include "Characters/GeoCharacter.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "EnhancedInputComponent.h"
#include "Kismet/GameplayStatics.h"
#include "VisualLogger/VisualLogger.h"

UGeoInputComponent::UGeoInputComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UGeoInputComponent::TickComponent(float DeltaSeconds, ELevelTick TickType,
									   FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaSeconds, TickType, ThisTickFunction);

	UpdateMouseLook();
}

void UGeoInputComponent::UpdateMouseLook()
{

	AGeoCharacter* GeoCharacter = GetGeoCharacter();
	if (!IsValid(GeoCharacter))
	{
		return;
	}

	APlayerController* const GeoPlayerController = GetGeoCharacter()->GetGeoController();
	if (!IsValid(GeoPlayerController))
	{
		return;
	}

	FVector2D ScreenPosition;
	ULocalPlayer* const LocalPlayer = GeoPlayerController->GetLocalPlayer();
	if (!IsValid(LocalPlayer) || !IsValid(LocalPlayer->ViewportClient)
		|| !LocalPlayer->ViewportClient->GetMousePosition(ScreenPosition))
	{
		return;
	}

	if (!ScreenPosition.Equals(LastMouseInput, 1.f))
	{
		LastMouseInput = ScreenPosition;
		GeoPlayerController->SetShowMouseCursor(true);
		FVector WorldLocation, WorldDirection;
		UGameplayStatics::DeprojectScreenToWorld(GeoPlayerController, ScreenPosition, WorldLocation, WorldDirection);

		FVector2D const LookDirection = FVector2d(WorldLocation - GeoCharacter->GetActorLocation());
		LastLookInput = LookDirection;
		GeoCharacter->DrawDebugVectorFromCharacter(
			FVector(LookDirection, 0.f), FString::Printf(TEXT("Look vector from %s"), *GeoCharacter->GetName()));
	}
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

void UGeoInputComponent::MoveFromInput(FInputActionInstance const& Instance)
{
	GetGeoCharacter()->AddMovementInput(FVector(Instance.GetValue().Get<FVector2D>(), 0.f));
}

void UGeoInputComponent::LookFromInput(FInputActionInstance const& Instance)
{
	FVector2D const LookInput = FVector2D(Instance.GetValue().Get<FVector2D>());
	GetGeoCharacter()->DrawDebugVectorFromCharacter(
		FVector(LookInput, 0.f), FString::Printf(TEXT("Look Input vector from %s"), *GetGeoCharacter()->GetName()));

	if (Instance.GetTriggerEvent() == ETriggerEvent::Completed)
	{
		LastLookInput = FVector2D::ZeroVector;
	}
	else
	{
		GetGeoCharacter()->GetGeoController()->SetShowMouseCursor(false);
		LastLookInput = LookInput;
	}
}

AGeoCharacter* UGeoInputComponent::GetGeoCharacter() const
{
	return Cast<AGeoCharacter>(GetOuter());
}
bool UGeoInputComponent::GetLookVector(FVector2D& OutLook) const
{
	OutLook = LastLookInput;
	return !LastLookInput.IsNearlyZero(ControllerDriftThreshold);
}
