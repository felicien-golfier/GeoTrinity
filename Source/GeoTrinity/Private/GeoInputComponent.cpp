// Fill out your copyright notice in the Description page of Project Settings.

#include "GeoInputComponent.h"

#include "EnhancedInputComponent.h"
#include "GameFramework/PlayerState.h"
#include "GeoInputGameInstanceSubsystem.h"
#include "GeoMovementComponent.h"
#include "GeoPawn.h"
#include "GeoPlayerController.h"
#include "GeoTrinity/GeoTrinity.h"
#include "InputStep.h"
#include "Kismet/GameplayStatics.h"

UGeoInputComponent::UGeoInputComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UGeoInputComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const AGeoPawn* GeoPawn = GetGeoPawn();
	const APlayerState* PlayerState = GeoPawn->GetPlayerState();
	if (!IsValid(GeoPawn) || !IsValid(PlayerState) || !GeoPawn->IsLocallyControlled())
	{
		return;
	}

	UGameplayStatics::GetAccurateRealTime(CurrentInputStep.Time.TimeSeconds, CurrentInputStep.Time.TimePartialSeconds);
	CurrentInputStep.DeltaTimeSeconds = DeltaTime;
	if (AGeoPlayerController* GeoPlayerController = Cast<AGeoPlayerController>(GeoPawn->GetController()))
	{
		CurrentInputStep.ServerTimeOffsetSeconds = GeoPlayerController->GetServerTimeOffsetSeconds();
	}
	SendInputServerRPC(CurrentInputStep);
	ProcessInput(CurrentInputStep);
	CurrentInputStep.Empty();
}

void UGeoInputComponent::BindInput(UInputComponent* PlayerInputComponent)
{
	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &UGeoInputComponent::MoveFromInput);
	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &UGeoInputComponent::MoveFromInput);
}
void UGeoInputComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void UGeoInputComponent::MoveFromInput(const FInputActionInstance& Instance)
{
	FVector2D AxisValues = Instance.GetValue().Get<FVector2D>();
	CurrentInputStep.MovementInput.X = AxisValues.X;
	CurrentInputStep.MovementInput.Y = AxisValues.Y;
}

AGeoPawn* UGeoInputComponent::GetGeoPawn() const
{
	return CastChecked<AGeoPawn>(GetOuter());
}

void UGeoInputComponent::ProcessInput(const FInputStep& InputStep)
{
	ProcessInput(InputStep, InputStep.DeltaTimeSeconds);
}

void UGeoInputComponent::ProcessInput(const FInputStep& InputStep, const float DeltaTime)
{
	if (DeltaTime <= 0.f)
	{
		UE_LOG(LogGeoTrinity, Error, TEXT("Incorrect delta time. (%f)."), DeltaTime);
		return;
	}

	AGeoPawn* GeoPawn = GetGeoPawn();
	if (UGeoMovementComponent* GeoMovementComponent = GeoPawn->GetGeoMovementComponent())
	{
		GeoMovementComponent->MovePawnWithInput(DeltaTime, InputStep.MovementInput);
	}
}

void UGeoInputComponent::SendInputServerRPC_Implementation(FInputStep InputStep)
{
	UGeoInputGameInstanceSubsystem::GetInstance(GetWorld())->ServerAddInputBuffer(InputStep, GetGeoPawn());
}

void UGeoInputComponent::SendForeignInputClientRPC_Implementation(const TArray<FInputAgent>& InputAgents)
{
	UGeoInputGameInstanceSubsystem::GetInstance(GetWorld())->ClientUpdateInputBuffer(InputAgents);
}