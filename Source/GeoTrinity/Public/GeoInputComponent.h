// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "GeoPawn.h"
#include "InputAction.h"
#include "InputStep.h"
#include "Input/GeoInputConfig.h"

#include "GeoInputComponent.generated.h"

class UGeoInputConfig;

UCLASS(Blueprintable, BlueprintType, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GEOTRINITY_API UGeoInputComponent : public UEnhancedInputComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UGeoInputComponent();
	void VLogCurrentInputStep(const AGeoPawn* GeoPawn) const;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;
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

	void ProcessInput(const FInputStep& InputStep);
	void ProcessInput(const FInputStep& InputStep, float DeltaTime);

	
	template<class UserClass, typename PressedFuncType, typename ReleasedFuncType, typename HeldFuncType>
	void BindAbilityActions(UserClass* object, PressedFuncType pressedFunc,	ReleasedFuncType releasedFunc, HeldFuncType heldFunc);
	
public:
	UPROPERTY(EditDefaultsOnly, Category = "Geo|Input")
	TObjectPtr<UInputAction> MoveAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "Geo|Input", meta=(ToolTip="Data that holds attached inputs"))
	TObjectPtr<UGeoInputConfig> InputConfig;

private:
	FInputStep CurrentInputStep;
};

template <class UserClass, typename PressedFuncType, typename ReleasedFuncType, typename HeldFuncType>
void UGeoInputComponent::BindAbilityActions(UserClass* object, PressedFuncType pressedFunc,
	ReleasedFuncType releasedFunc, HeldFuncType heldFunc)
{
	checkf(InputConfig, TEXT("Please fill in Input config in %s"), *GetName());
	
	for (FGeoInputAction const& action : InputConfig->AbilityInputActions)
	{
		if (!(action.InputAction && action.InputTag.IsValid()))
			continue;
		
		if (pressedFunc)
		{
			BindAction(action.InputAction, ETriggerEvent::Started, object, pressedFunc, action.InputTag);
		}
		if (releasedFunc)
		{
			BindAction(action.InputAction, ETriggerEvent::Completed, object, releasedFunc, action.InputTag);
		}
		if (heldFunc)
		{
			// Triggered means the function is called every frame as long as the button is pressed (Start is only once)
			BindAction(action.InputAction, ETriggerEvent::Triggered, object, heldFunc, action.InputTag);
		}
	}
}
