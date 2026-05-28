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
	/** Creates the ASC and attribute set as direct subobjects (enemies own their ASC, unlike players who use PlayerState). Sets AGeoEnemyAIController as the AI controller class. */
	AEnemyCharacter(FObjectInitializer const& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UStateTree> StateTree;

protected:
	virtual void BeginPlay() override;

	virtual void InitGAS() override;

	UFUNCTION(BlueprintNativeEvent)
	void OnHealthChanged(float NewValue);
	virtual void OnHealthChanged_Implementation(float NewValue);

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Enemy")
	bool ResetToFullLifeWhenReachingZero = false;
};
