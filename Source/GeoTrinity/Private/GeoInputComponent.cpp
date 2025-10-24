// Fill out your copyright notice in the Description page of Project Settings.

#include "GeoInputComponent.h"

#include "EnhancedInputComponent.h"
#include "GameFramework/PlayerState.h"
#include "GeoInputGameInstanceSubsystem.h"
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

	UGameplayStatics::GetAccurateRealTime(CurrentInputStep.TimeSeconds, CurrentInputStep.TimePartialSeconds);
	CurrentInputStep.Ping = PlayerState->GetPingInMilliseconds();
	if (AGeoPlayerController* GeoPC = Cast<AGeoPlayerController>(GeoPawn->GetController()))
	{
		CurrentInputStep.ServerTimeSeconds = GeoPC->GetEstimatedServerTimeSeconds();
	}
	SendInputServerRPC(CurrentInputStep);
	ProcessInput(CurrentInputStep);
	CurrentInputStep.Empty();
}

void UGeoInputComponent::BindInput(UInputComponent* PlayerInputComponent)
{
	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &UGeoInputComponent::Move);
	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &UGeoInputComponent::Move);
}
void UGeoInputComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void UGeoInputComponent::Move(const FInputActionInstance& Instance)
{
	FVector2D AxisValues = Instance.GetValue().Get<FVector2D>();
	CurrentInputStep.MovementInput.X = AxisValues.X;
	CurrentInputStep.MovementInput.Y = AxisValues.Y;
}

void UGeoInputComponent::UpdateCharacterLocation(float DeltaTime, FVector2D GivenMovementInput) const
{
	if (GivenMovementInput.IsZero())
	{
		return;
	}

	// Scale our movement input axis values by 100 units per second
	GivenMovementInput = GivenMovementInput.GetSafeNormal() * 100.0f;
	AGeoPawn* GeoPawn = GetGeoPawn();
	FVector NewLocation = GeoPawn->GetActorLocation();

	NewLocation += GeoPawn->GetActorForwardVector() * GivenMovementInput.Y * DeltaTime;
	NewLocation += GeoPawn->GetActorRightVector() * GivenMovementInput.X * DeltaTime;
	GeoPawn->GetBox().Position = FVector2D(NewLocation);
	GeoPawn->SetActorLocation(NewLocation);
}

AGeoPawn* UGeoInputComponent::GetGeoPawn() const
{
	return CastChecked<AGeoPawn>(GetOuter());
}

void UGeoInputComponent::ApplyCollision(const FGeoBox& Obstacle) const
{
	const AGeoPawn* GeoPawn = GetGeoPawn();
	FGeoBox Box = GeoPawn->GetBox();
	if (!Box.Overlaps(Obstacle))
	{
		return;
	}

	// Correction X
	if (Box.Position.X < Obstacle.Position.X)
	{
		Box.Position.X = Obstacle.Position.X - Obstacle.Size.X - Box.Size.X;
	}
	else
	{
		Box.Position.X = Obstacle.Position.X + Obstacle.Size.X + Box.Size.X;
	}

	// Correction Y
	if (Box.Position.Y < Obstacle.Position.Y)
	{
		Box.Position.Y = Obstacle.Position.Y - Obstacle.Size.Y - Box.Size.Y;
	}
	else
	{
		Box.Position.Y = Obstacle.Position.Y + Obstacle.Size.Y + Box.Size.Y;
	}
}

void UGeoInputComponent::ProcessInput(const FInputStep& InputStep)
{
	float DeltaTime = PreviousInputStepProcessed.IsEmpty() ? 0.017f : InputStep.GetTimeDiff(PreviousInputStepProcessed);
	if (DeltaTime <= 0.f)
	{
		UE_LOG(LogGeoTrinity, Warning, TEXT("Incorrect delta time. (%f)."), DeltaTime);
		DeltaTime = 0.017f;
	}

	// Always update with a valid DeltaTime
	UpdateCharacterLocation(DeltaTime, InputStep.MovementInput);

	PreviousInputStepProcessed = InputStep;
}

void UGeoInputComponent::SendInputServerRPC_Implementation(FInputStep InputStep)
{
	UGeoInputGameInstanceSubsystem::GetInstance(GetWorld())->ServerAddInputBuffer(InputStep, GetGeoPawn());
}

void UGeoInputComponent::SendForeignInputClientRPC_Implementation(const TArray<FInputAgent>& InputAgents)
{
	UGeoInputGameInstanceSubsystem::GetInstance(GetWorld())->ClientUpdateInputBuffer(InputAgents);
}