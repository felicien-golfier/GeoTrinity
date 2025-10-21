// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "GeoPawn.h"
#include "InputAction.h"

#include "GeoMovementComponent.generated.h"

UCLASS(Blueprintable, BlueprintType, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GEOTRINITY_API UGeoMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UGeoMovementComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void BindInput(UInputComponent* PlayerInputComponent);

	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;

	UFUNCTION()
	void Move(const FInputActionInstance& Instance);

	void UpdateCharacterLocation(float DeltaTime, FVector2D GivenMovementInput) const;
	AGeoPawn* GetGeoPawn() const;
	void ApplyCollision(const FGeoBox& Obstacle) const;

	UFUNCTION()
	void OnRep_MovementInput();

	// Server RPC Function
	UFUNCTION(Server, reliable)
	void ServerRPC(FVector2D MovementInputChanged);

private:
	UPROPERTY(ReplicatedUsing = OnRep_MovementInput)
	FVector2D MovementInput;

public:
	UPROPERTY(EditDefaultsOnly, Category = "GeoMovementComponent|Input")
	TObjectPtr< UInputAction > MoveAction;
};
