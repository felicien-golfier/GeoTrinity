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
	virtual void TickComponent(float DeltaSeconds, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	void UpdateMouseLook();
	void BindInput(UInputComponent* PlayerInputComponent);

	UFUNCTION()
	void MoveFromInput(const FInputActionInstance& Instance);

	UFUNCTION()
	void LookFromInput(const FInputActionInstance& Instance);

	AGeoCharacter* GetGeoCharacter() const;

	// Returns true if there is a valid non-zero look vector from right stick
	bool GetLookVector(FVector2D& OutLook) const;

	template<class UserClass, typename PressedFuncType, typename ReleasedFuncType, typename HeldFuncType>
	void BindAbilityActions(UserClass* object, PressedFuncType pressedFunc, ReleasedFuncType releasedFunc,
		HeldFuncType heldFunc);

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
	UPROPERTY(Transient)
	FVector2D LastMouseLookInput = FVector2D::ZeroVector;

	UPROPERTY(Transient)
	FVector2D LastMouseInput = FVector2D::ZeroVector;

	constexpr static float ControllerDriftThreshold = 0.1f;
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
