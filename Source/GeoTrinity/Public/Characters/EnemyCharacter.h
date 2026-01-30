// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Characters/GeoCharacter.h"
#include "CoreMinimal.h"
#include "UObject/SoftObjectPtr.h"

#include "EnemyCharacter.generated.h"

/**
 *
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

	// Behavior Tree to run for this enemy (assigned per instance or via BP)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|AI")
	TObjectPtr<class UBehaviorTree> BehaviorTree;

	// Get next firing point (cycles) and advance the internal index. Returns false if none.
	bool GetAndAdvanceNextFiringPointLocation(FVector& OutLocation);

protected:
	virtual void BeginPlay() override;

	virtual void InitGAS() override;

	UFUNCTION()
	void OnHealthChanged(float NewValue);

	// Index used for round-robin selection of firing points
	int CurrentFiringPointIndex = 0;

private:
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"), Category = "Enemy")
	bool ResetToFullLifeWhenReachingZero = false;
};
