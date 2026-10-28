// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystem/Abilities/Base/AbilityPayload.h"
#include "CoreMinimal.h"
#include "Encounter/GeoEncounterAction.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"

#include "GeoActivateAbilityAction.generated.h"

class UAbilitySystemComponent;
class UGeoGameplayAbility;

/**
 * Fires AbilityClass on the encounters' shared caster (UGeoEncounterSubsystem::GetSharedCaster), with ArenaTag and
 * PatternData as its FPatternAbilityParams. Finishes when the ability ends (bWaitForEnd) or at once. A refused
 * activation is logged and finishes, so the encounter never stalls on it.
 */
UCLASS()
class GEOTRINITY_API UGeoActivateAbilityAction : public UGeoEncounterAction
{
	GENERATED_BODY()

public:
	UGeoActivateAbilityAction() { bLatent = true; }

#if WITH_EDITOR
	/** Flags an empty AbilityClass and one whose CDO RequiresArena (the shared caster refuses it). */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	virtual void Execute_Implementation(FGeoEncounterPayload const& Payload) override;

	/** Cancels the ability still waited for and unbinds. On abort it also cancels the one it fired without waiting
	 * (bWaitForEnd false), so a wave never runs on in the room left behind. */
	virtual void OnCancel_Implementation() override;

	/** The ability fired; granted to the shared caster on first use. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	TSubclassOf<UGeoGameplayAbility> AbilityClass;

	/** The room the ability fires in: its points (an ability's TargetPointTag) are the ones carrying this tag. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter", meta = (Categories = "Arena"))
	FGameplayTag ArenaTag;

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
