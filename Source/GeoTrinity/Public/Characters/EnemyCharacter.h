// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Characters/GeoCharacter.h"
#include "CoreMinimal.h"
#include "UObject/SoftObjectPtr.h"

#include "EnemyCharacter.generated.h"

class UStateTree;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBossDefeated);

/**
 * Enemy character controlled by a StateTree AI. Owns its ASC as a direct subobject (unlike
 * APlayableCharacter, where GAS lives on PlayerState). Boss death is handled in OnHealthChanged:
 * either resets health for looping attempts or broadcasts OnBossDefeated and self-destructs.
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

	UPROPERTY(BlueprintAssignable, Category = "Boss")
	FOnBossDefeated OnBossDefeated;

	/** Resets health to max and restarts the StateTree. Called on full-wipe to start a new attempt. */
	void ResetForNewAttempt();

protected:
	virtual void BeginPlay() override;

	virtual void InitGAS() override;

	UFUNCTION(BlueprintNativeEvent)
	void OnHealthChanged(float NewValue);
	virtual void OnHealthChanged_Implementation(float NewValue);

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Enemy")
	bool ResetToFullLifeWhenReachingZero = false;

	bool bIsResetting = false;
};
