// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "GeoPawn.h"
#include "Input/GeoInputConfig.h"
#include "InputAction.h"

#include "GeoInputComponent.generated.h"

UCLASS(Blueprintable, BlueprintType, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GEOTRINITY_API UGeoInputComponent : public UEnhancedInputComponent
{
	GENERATED_BODY()

	friend class AGeoPlayerController;

public:
	// Sets default values for this component's properties
	UGeoInputComponent();

	void BindInput(UInputComponent* PlayerInputComponent);

	UFUNCTION()
	void MoveFromInput(const FInputActionInstance& Instance);

	AGeoPawn* GetGeoPawn() const;

	template<class UserClass, typename PressedFuncType, typename ReleasedFuncType, typename HeldFuncType>
	void BindAbilityActions(UserClass* object, PressedFuncType pressedFunc, ReleasedFuncType releasedFunc,
		HeldFuncType heldFunc);

public:
	UPROPERTY(EditDefaultsOnly, Category = "Geo|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Geo|Input", meta = (ToolTip = "Data that holds attached inputs"))
	TObjectPtr<UGeoInputConfig> InputConfig;
};

// template function definition must be in .h
// because the compiler must see its full definition in every translation unit
template<class UserClass, typename PressedFuncType, typename ReleasedFuncType, typename HeldFuncType>
void UGeoInputComponent::BindAbilityActions(UserClass* object, PressedFuncType pressedFunc,
	ReleasedFuncType releasedFunc, HeldFuncType heldFunc)
{
	checkf(InputConfig, TEXT("Please fill in Input config in %s"), *GetName());

	for (const FGeoInputAction& action : InputConfig->AbilityInputActions)
	{
		if (!(action.InputAction && action.InputTag.IsValid()))
		{
			continue;
		}

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
