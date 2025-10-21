// Fill out your copyright notice in the Description page of Project Settings.

#include "GeoMovementComponent.h"

#include "EnhancedInputComponent.h"
#include "GeoPawn.h"
#include "Net/UnrealNetwork.h"

UGeoMovementComponent::UGeoMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetComponentTickEnabled(true);
	SetIsReplicatedByDefault(true);
}

void UGeoMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateCharacterLocation(DeltaTime, MovementInput);
	ServerRPC(MovementInput);
}

void UGeoMovementComponent::BindInput(UInputComponent* PlayerInputComponent)
{
	UEnhancedInputComponent* EnhancedInputComponent = CastChecked< UEnhancedInputComponent >(PlayerInputComponent);
	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &UGeoMovementComponent::Move);
	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &UGeoMovementComponent::Move);
}
void UGeoMovementComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGeoMovementComponent, MovementInput);
}

void UGeoMovementComponent::Move(const FInputActionInstance& Instance)
{
	FVector2D AxisValues = Instance.GetValue().Get< FVector2D >();
	MovementInput.X = AxisValues.X;
	MovementInput.Y = AxisValues.Y;
}

void UGeoMovementComponent::UpdateCharacterLocation(float DeltaTime, FVector2D GivenMovementInput) const
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

AGeoPawn* UGeoMovementComponent::GetGeoPawn() const
{
	return CastChecked< AGeoPawn >(GetOuter());
}

void UGeoMovementComponent::ApplyCollision(const FGeoBox& Obstacle) const
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

void UGeoMovementComponent::OnRep_MovementInput()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Black,
		FString::Printf(TEXT("RepMomentInput : %f from actor %s"), MovementInput.X, *GetGeoPawn()->GetName()));
}
void UGeoMovementComponent::ServerRPC_Implementation(FVector2D MovementInputChanged)
{
	MovementInput = MovementInputChanged;
}