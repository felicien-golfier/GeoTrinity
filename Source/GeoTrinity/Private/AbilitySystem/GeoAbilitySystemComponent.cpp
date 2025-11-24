// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/GeoAbilitySystemComponent.h"

#include "AbilitySystem/GeoAscTypes.h"
#include "AbilitySystem/Abilities/GeoGameplayAbility.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "GeoTrinity/GeoTrinity.h"

FGeoGameplayEffectContext* UGeoAbilitySystemComponent::MakeGeoEffectContext() const
{
	FGameplayEffectContextHandle handle = MakeEffectContext();
	return static_cast<FGeoGameplayEffectContext*>(handle.Get());
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySystemComponent::AddCharacterStartupAbilities(TArray<TSubclassOf<UGeoGameplayAbility>>& AbilitiesToGive)
{
	for(TSubclassOf<UGeoGameplayAbility> const& AbilityClass:AbilitiesToGive)
	{
		FGameplayAbilitySpec abilitySpec {AbilityClass, 1};
        if (UGeoGameplayAbility const* pMMAbility = Cast<UGeoGameplayAbility>(abilitySpec.Ability))
        {
	        abilitySpec.GetDynamicSpecSourceTags().AddTag(pMMAbility->StartupInputTag);
        	// If we ever need to be able to switch between active GA, we will need to use this system again. Leaving it as a reminder
        	//abilitySpec.GetDynamicSpecSourceTags().AddTag(tags.Abilities_Status_Equipped);
        }
        
		GiveAbility(abilitySpec);
	}
	bStartupAbilitiesGiven = true;
	// Event broadcasting will be needed for UI
	//AbilitiesGivenEvent.Broadcast();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySystemComponent::AbilityInputTagPressed(FGameplayTag const& inputTag)
{
	if(!inputTag.IsValid())
		return;

	UE_VLOG(this, LogGeoASC, VeryVerbose, TEXT("AbilityInputTag Pressed of INPUT %s"), *inputTag.ToString());
    
	FScopedAbilityListLock activeScopeLock(*this);
	for(FGameplayAbilitySpec& abilitySpec : GetActivatableAbilities())
	{
		// Only activate ability of given input tag
		if (!abilitySpec.GetDynamicSpecSourceTags().HasTagExact(inputTag))
			continue;

		AbilitySpecInputPressed(abilitySpec);
		if (abilitySpec.IsActive())
		{
			PRAGMA_DISABLE_DEPRECATION_WARNINGS
			// Code from Lyra starter game (if they disable Deprecation warnings, I don't see why not do the same)
			const UGameplayAbility* Instance = abilitySpec.GetPrimaryInstance();
			FPredictionKey originalPredictionKey = Instance ? Instance->GetCurrentActivationInfo().GetActivationPredictionKey() : abilitySpec.ActivationInfo.GetActivationPredictionKey();
			PRAGMA_ENABLE_DEPRECATION_WARNINGS
            
			InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, abilitySpec.Handle, originalPredictionKey);
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySystemComponent::AbilityInputTagHeld(FGameplayTag const& inputTag)
{
	if(!inputTag.IsValid())
		return;

	FScopedAbilityListLock activeScopeLock(*this);
	for(FGameplayAbilitySpec& abilitySpec : GetActivatableAbilities())
	{
		if (!abilitySpec.GetDynamicSpecSourceTags().HasTagExact(inputTag))
			continue;
		
		AbilitySpecInputPressed(abilitySpec);
		if (!abilitySpec.IsActive())
		{
			TryActivateAbility(abilitySpec.Handle);
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySystemComponent::AbilityInputTagReleased(FGameplayTag const& inputTag)
{
	if(!inputTag.IsValid())
		return;

	FScopedAbilityListLock activeScopeLock(*this);
	for(FGameplayAbilitySpec& abilitySpec : GetActivatableAbilities())
	{
		if (abilitySpec.GetDynamicSpecSourceTags().HasTagExact(inputTag) && abilitySpec.IsActive())
		{
			AbilitySpecInputReleased(abilitySpec);
            
			PRAGMA_DISABLE_DEPRECATION_WARNINGS
			// Code from Lyra starter game (if they disable Deprecation warnings, I don't see why not do the same)
			const UGameplayAbility* Instance = abilitySpec.GetPrimaryInstance();
			FPredictionKey originalPredictionKey = Instance ? Instance->GetCurrentActivationInfo().GetActivationPredictionKey() : abilitySpec.ActivationInfo.GetActivationPredictionKey();
			PRAGMA_ENABLE_DEPRECATION_WARNINGS

			// Needed to use Wait for input release in blueprint
			InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, abilitySpec.Handle, originalPredictionKey);
		}
	}
}