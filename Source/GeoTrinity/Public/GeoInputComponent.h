// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "GeoInputGameInstanceSubsystem.h"
#include "GeoPawn.h"
#include "InputAction.h"
#include "InputStep.h"

#include "GeoInputComponent.generated.h"

UCLASS(Blueprintable, BlueprintType, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GEOTRINITY_API UGeoInputComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UGeoInputComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void BindInput(UInputComponent* PlayerInputComponent);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void MoveFromInput(const FInputActionInstance& Instance);

	AGeoPawn* GetGeoPawn() const;

	// Server RPC Function
	UFUNCTION(Server, reliable)
	void SendInputServerRPC(FInputStep InputStep);

	UFUNCTION(Client, reliable)
	void SendForeignInputClientRPC(const TArray<FInputAgent>& InputAgents);

	void ProcessInput(const FInputStep& InputStep, const float DeltaTime);

public:
	UPROPERTY(EditDefaultsOnly, Category = "Geo|Input")
	TObjectPtr<UInputAction> MoveAction;

private:
	FInputStep CurrentInputStep;
};
