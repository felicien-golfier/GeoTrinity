// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Input/GeoInputComponent.h"

#include "Characters/GeoCharacter.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "EnhancedInputComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Tool/UGeoGameplayLibrary.h"
#include "VisualLogger/VisualLogger.h"
#include "World/GeoGameCamera.h"

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

	AGeoPlayerController* const GeoPlayerController = GetGeoCharacter()->GetGeoPlayerController();
	if (!IsValid(GeoPlayerController) || GeoPlayerController->IsPauseMenuOpen())
	{
		return;
	}

	// There is one mouse: every other couch-coop player aims with the right stick only, and never sees the cursor
	// steal their aim.
	if (!GeoLib::IsKeyboardMousePlayer(GeoPlayerController))
	{
		bIsUsingController = true;
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
		bIsUsingController = false;
		GeoPlayerController->SetMouseCursorVisible(true);
		LastMouseInput = ScreenPosition;
	}

	if (!bIsUsingController)
	{
		FVector WorldLocation, WorldDirection;
		UGameplayStatics::DeprojectScreenToWorld(GeoPlayerController, ScreenPosition, WorldLocation, WorldDirection);

		FVector2D const LookDirection = FVector2d(WorldLocation - GeoCharacter->GetActorLocation());
		LastLookInput = LookDirection;
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

	if (ZoomAction)
	{
		EnhancedInputComponent->BindAction(ZoomAction, ETriggerEvent::Triggered, this,
										   &UGeoInputComponent::ZoomFromInput);
	}
}

void UGeoInputComponent::MoveFromInput(FInputActionInstance const& Instance)
{
	if (AGeoCharacter* GeoCharacter = GetGeoCharacter())
	{
		GeoCharacter->AddMovementInput(FVector(Instance.GetValue().Get<FVector2D>(), 0.f));
	}
}

void UGeoInputComponent::LookFromInput(FInputActionInstance const& Instance)
{
	bIsUsingController = true;
	FVector2D const LookInput = FVector2D(Instance.GetValue().Get<FVector2D>());

	if (Instance.GetTriggerEvent() == ETriggerEvent::Completed)
	{
		LastLookInput = FVector2D::ZeroVector;
	}
	else
	{
		AGeoCharacter* GeoCharacter = GetGeoCharacter();
		AGeoPlayerController* PlayerController = GeoCharacter ? GeoCharacter->GetGeoPlayerController() : nullptr;
		if (PlayerController)
		{
			PlayerController->SetMouseCursorVisible(false);
		}
		LastLookInput = LookInput;
	}
}

void UGeoInputComponent::ZoomFromInput(FInputActionInstance const& Instance)
{
	if (AGeoGameCamera* const Camera =
			Cast<AGeoGameCamera>(UGameplayStatics::GetActorOfClass(this, AGeoGameCamera::StaticClass())))
	{
		Camera->AddZoomInput(Instance.GetValue().Get<float>());
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
