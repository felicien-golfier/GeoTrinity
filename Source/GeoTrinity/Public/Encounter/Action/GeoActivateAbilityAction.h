// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Abilities/GameplayAbilityTypes.h"
#include "CoreMinimal.h"
#include "Encounter/GeoEncounterAction.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"

#include "GeoActivateAbilityAction.generated.h"

class AGeoArena;
class UAbilitySystemComponent;

/**
 * Activates AbilityTag on the boss of Arena: a room's hidden caster, or a real boss. Finishes when the ability ends
 * (bWaitForEnd) or at once. A refused activation is logged and finishes, so the encounter never stalls on it.
 */
UCLASS()
class GEOTRINITY_API UGeoActivateAbilityAction : public UGeoEncounterAction
{
	GENERATED_BODY()

public:
	UGeoActivateAbilityAction() { bLatent = true; }

	virtual void Execute_Implementation(FGeoEncounterPayload const& Payload) override;

	/** Cancels the ability still running and unbinds. */
	virtual void OnCancel_Implementation() override;

	/** The caster is spawned at runtime, so the action points at its arena. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	TObjectPtr<AGeoArena> Arena;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	FGameplayTag AbilityTag;

	/** True: Finish when the ability ends. False: Finish right after activation (overlapping waves). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	bool bWaitForEnd = true;

private:
	void OnAbilityEnded(FAbilityEndedData const& EndedData);

	/** Unbinds from the ASC and forgets the ability. */
	void StopWaiting();

	TWeakObjectPtr<UAbilitySystemComponent> WaitedASC;
	FGameplayAbilitySpecHandle WaitedHandle;
	FDelegateHandle AbilityEndedDelegateHandle;
};
