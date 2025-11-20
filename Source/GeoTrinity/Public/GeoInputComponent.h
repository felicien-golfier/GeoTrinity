// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "Input/GeoInputConfig.h"
#include "InputAction.h"

struct FInputActionInstance;
class AGeoCharacter;

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

	UFUNCTION()
	void LookFromInput(const FInputActionInstance& Instance);

	AGeoCharacter* GetGeoCharacter() const;

	// Returns true if there is a valid non-zero look vector from right stick
	bool GetLookVector(FVector2D& OutLook) const
	{
		OutLook = LastLookInput;
		return !LastLookInput.IsNearlyZero(0.05f);
	}

	template<class UserClass, typename PressedFuncType, typename ReleasedFuncType, typename HeldFuncType>
	void BindAbilityActions(UserClass* Object, PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc,
		HeldFuncType HeldFunc);

public:
	UPROPERTY(EditDefaultsOnly, Category = "Geo|Input")
	TObjectPtr<UInputAction> MoveAction;

	// Right stick / mouse delta look action (Value: Vector2D). Assign in the Input Mapping.
	UPROPERTY(EditDefaultsOnly, Category = "Geo|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Geo|Input", meta = (ToolTip = "Data that holds attached inputs"))
	TObjectPtr<UGeoInputConfig> InputConfig;

private:
	// Cached latest right stick vector in viewport space (X,Y), not normalized. Zero when idle.
	UPROPERTY(Transient)
	FVector2D LastLookInput = FVector2D::ZeroVector;
};

// template function definition must be in .h
// because the compiler must see its full definition in every translation unit
template<class UserClass, typename PressedFuncType, typename ReleasedFuncType, typename HeldFuncType>
void UGeoInputComponent::BindAbilityActions(UserClass* Object, PressedFuncType PressedFunc,
	ReleasedFuncType ReleasedFunc, HeldFuncType HeldFunc)
{
	checkf(InputConfig, TEXT("Please fill in Input config in %s"), *GetName());

	for (const auto& [InputAction, InputTag] : InputConfig->AbilityInputActions)
	{
		if (!(InputAction && InputTag.IsValid()))
		{
			continue;
		}

		if (PressedFunc)
		{
			BindAction(InputAction, ETriggerEvent::Started, Object, PressedFunc, InputTag);
		}
		if (ReleasedFunc)
		{
			BindAction(InputAction, ETriggerEvent::Completed, Object, ReleasedFunc, InputTag);
		}
		if (HeldFunc)
		{
			// Triggered means the function is called every frame as long as the button is pressed (Start is only once)
			BindAction(InputAction, ETriggerEvent::Triggered, Object, HeldFunc, InputTag);
		}
	}
}
