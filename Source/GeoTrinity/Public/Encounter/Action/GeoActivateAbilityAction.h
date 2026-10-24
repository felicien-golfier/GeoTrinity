// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystem/Abilities/Base/PatternAbility.h"
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
 * PatternData configures a pattern ability for this firing.
 */
UCLASS()
class GEOTRINITY_API UGeoActivateAbilityAction
	: public UGeoEncounterAction
	, public IPatternDataSource
{
	GENERATED_BODY()

public:
	UGeoActivateAbilityAction() { bLatent = true; }

	virtual void Execute_Implementation(FGeoEncounterPayload const& Payload) override;

	/** Cancels the ability still waited for and unbinds. On abort it also cancels the one it fired without waiting
	 * (bWaitForEnd false), so a wave never runs on in the room left behind. */
	virtual void OnCancel_Implementation() override;

	virtual TInstancedStruct<FPatternData> GetPatternData() const override { return PatternData; }

	/** The caster is spawned at runtime, so the action points at its arena. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	TObjectPtr<AGeoArena> Arena;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	FGameplayTag AbilityTag;

	/** True: Finish when the ability ends. False: Finish right after activation (overlapping waves). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	bool bWaitForEnd = true;

	/** Pattern data a pattern ability launches this firing with (a lateral wave's safe spots). Unset: the ability's
	 * own.
	 */
	UPROPERTY(EditAnywhere, Category = "GeoEncounter")
	TInstancedStruct<FPatternData> PatternData;

private:
	void OnAbilityEnded(FAbilityEndedData const& EndedData);

	/** Unbinds from the ASC and forgets the ability. */
	void StopWaiting();

	/** The ability the last Execute activated, until it ends (waited) or the action is cancelled. */
	TWeakObjectPtr<UAbilitySystemComponent> ActivatedASC;
	FGameplayAbilitySpecHandle ActivatedHandle;
	/** Valid while waiting for the activated ability's end (bWaitForEnd). */
	FDelegateHandle AbilityEndedDelegateHandle;
};
