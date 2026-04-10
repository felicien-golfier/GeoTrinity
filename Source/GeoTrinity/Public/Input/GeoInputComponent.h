// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Data/AbilityInfo.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "GeoTrinity/GeoTrinity.h"
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
	UGeoInputComponent();
	virtual void TickComponent(float DeltaSeconds, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;

	/** Updates the character's aim rotation from the latest mouse or right-stick look input. */
	void UpdateMouseLook();

	/** Binds movement, look, and all ability input actions to their respective callbacks. */
	void BindInput(UInputComponent* PlayerInputComponent);

	/** Callback for the move input action. Applies directional movement to the owning character. */
	UFUNCTION()
	void MoveFromInput(FInputActionInstance const& Instance);

	/** Callback for the look input action. Caches the latest look vector for rotation processing. */
	UFUNCTION()
	void LookFromInput(FInputActionInstance const& Instance);

	/** Returns the GeoCharacter that owns this component, or nullptr if the owner is not a GeoCharacter. */
	AGeoCharacter* GetGeoCharacter() const;

	/**
	 * Returns the latest non-zero look vector from the right stick.
	 *
	 * @param OutLook  Set to the cached look vector in viewport space.
	 * @return         True if the vector is non-zero (stick is active).
	 */
	bool GetLookVector(FVector2D& OutLook) const;

	/**
	 * Binds started/completed/triggered events for every ability action defined in AbilityInfo.
	 * Deduplicates (InputAction, InputTag) pairs so the same action is bound only once per event type.
	 *
	 * @param Object         Object that owns the callback functions.
	 * @param PressedFunc    Called once when the action starts. Pass nullptr to skip.
	 * @param ReleasedFunc   Called once when the action completes. Pass nullptr to skip.
	 * @param HeldFunc       Called every frame while the action is triggered. Pass nullptr to skip.
	 * @param AbilityInfo    Data asset listing all player ability input mappings.
	 */
	template <class UserClass, typename PressedFuncType, typename ReleasedFuncType, typename HeldFuncType>
	void BindAbilityActions(UserClass* Object, PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc,
							HeldFuncType HeldFunc, UAbilityInfo* AbilityInfo);

	UPROPERTY(EditDefaultsOnly, Category = "Geo|Input")
	TObjectPtr<UInputAction> MoveAction;

	// Right stick / mouse delta look action (Value: Vector2D). Assign in the Input Mapping.
	UPROPERTY(EditDefaultsOnly, Category = "Geo|Input")
	TObjectPtr<UInputAction> LookAction;

private:
	// Cached latest right stick vector in viewport space (X,Y), not normalized. Zero when idle.
	FVector2D LastLookInput = FVector2D::ZeroVector;
	FVector2D LastMouseInput = FVector2D::ZeroVector;
	bool bIsUsingController = false;
	constexpr static float ControllerDriftThreshold = 0.1f;
};

// template function definition must be in .h
// because the compiler must see its full definition in every translation unit
template <class UserClass, typename PressedFuncType, typename ReleasedFuncType, typename HeldFuncType>
void UGeoInputComponent::BindAbilityActions(UserClass* Object, PressedFuncType PressedFunc,
											ReleasedFuncType ReleasedFunc, HeldFuncType HeldFunc,
											UAbilityInfo* AbilityInfo)
{
	checkf(AbilityInfo, TEXT("AbilityInfo is null in BindAbilityActions on %s"), *GetName());

	TSet<TTuple<UInputAction*, FGameplayTag>> BoundPairs;

	for (FPlayersGameplayAbilityInfo const& Info : AbilityInfo->GetAllPlayersAbilityInfos())
	{
		if (!Info.InputAction || !Info.InputTag.IsValid())
		{
			if (Info.InputAction || Info.InputTag.IsValid())
			{
				ensureMsgf(false, TEXT("Ability Info has not filled all input in the Data !"));
			}

			continue;
		}

		TTuple<UInputAction*, FGameplayTag> Pair{Info.InputAction, Info.InputTag};
		if (BoundPairs.Contains(Pair))
		{
			continue;
		}
		BoundPairs.Add(Pair);

		if (PressedFunc)
		{
			BindAction(Info.InputAction, ETriggerEvent::Started, Object, PressedFunc, Info.InputTag);
		}
		if (ReleasedFunc)
		{
			BindAction(Info.InputAction, ETriggerEvent::Completed, Object, ReleasedFunc, Info.InputTag);
		}
		if (HeldFunc)
		{
			// Triggered means the function is called every frame as long as the button is pressed (Start is only once)
			BindAction(Info.InputAction, ETriggerEvent::Triggered, Object, HeldFunc, Info.InputTag);
		}
	}
}
