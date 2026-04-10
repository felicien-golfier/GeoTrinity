// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Characters/GeoCharacter.h"
#include "CoreMinimal.h"
#include "UObject/SoftObjectPtr.h"

#include "EnemyCharacter.generated.h"

class UStateTree;
/**
 * Enemy character controlled by a StateTree AI. Owns its own ASC directly (no PlayerState).
 * Maintains an ordered list of firing points gathered from world actors tagged "Path" and cycles
 * through them round-robin to position itself before activating abilities.
 */
UCLASS()
class GEOTRINITY_API AEnemyCharacter : public AGeoCharacter
{
	GENERATED_BODY()

public:
	AEnemyCharacter(FObjectInitializer const& ObjectInitializer);

	// Firing points the enemy will move to (round-robin) to cast abilities
	UPROPERTY(Transient)
	TArray<AActor*> FiringPoints;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UStateTree> StateTree;

	/**
	 * Returns the world location of the next firing point and advances the internal round-robin index.
	 *
	 * @param OutLocation  Set to the next firing point's world location if one exists.
	 * @return             False if FiringPoints is empty.
	 */
	bool GetAndAdvanceNextFiringPointLocation(FVector& OutLocation);

protected:
	virtual void BeginPlay() override;

	virtual void InitGAS() override;

	UFUNCTION(BlueprintNativeEvent)
	void OnHealthChanged(float NewValue);
	virtual void OnHealthChanged_Implementation(float NewValue);

	// Index used for round-robin selection of firing points
	int CurrentFiringPointIndex = 0;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Enemy")
	bool ResetToFullLifeWhenReachingZero = false;
};
