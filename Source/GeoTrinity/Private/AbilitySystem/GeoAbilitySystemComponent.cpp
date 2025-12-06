// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/GeoAbilitySystemComponent.h"

#include "AbilitySystem/Abilities/GeoGameplayAbility.h"
#include "AbilitySystem/GeoAscTypes.h"
#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "GeoTrinity/GeoTrinity.h"

void UGeoAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);
	
	BindAttributeCallbacks();
}

// ---------------------------------------------------------------------------------------------------------------------
FGeoGameplayEffectContext* UGeoAbilitySystemComponent::MakeGeoEffectContext() const
{
	FGameplayEffectContextHandle handle = MakeEffectContext();
	return static_cast<FGeoGameplayEffectContext*>(handle.Get());
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySystemComponent::AddCharacterStartupAbilities(TArray<TSubclassOf<UGeoGameplayAbility>>& AbilitiesToGive)
{
	for (const TSubclassOf<UGeoGameplayAbility>& AbilityClass : AbilitiesToGive)
	{
		FGameplayAbilitySpec abilitySpec{AbilityClass, 1};
		if (const UGeoGameplayAbility* pMMAbility = Cast<UGeoGameplayAbility>(abilitySpec.Ability))
		{
			abilitySpec.GetDynamicSpecSourceTags().AddTag(pMMAbility->StartupInputTag);
			// If we ever need to be able to switch between active GA, we will need to use this system again. Leaving it
			// as a reminder
			// abilitySpec.GetDynamicSpecSourceTags().AddTag(tags.Abilities_Status_Equipped);
		}

		GiveAbility(abilitySpec);
	}
	bStartupAbilitiesGiven = true;
	// Event broadcasting will be needed for UI
	// AbilitiesGivenEvent.Broadcast();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySystemComponent::AddCharacterStartupAbilities(TArray<FGameplayTag> const& AbilitiesToGive, const int32 Level /*= 1.f*/)
{
	UAbilityInfo* AbilityInfos = UGeoAbilitySystemLibrary::GetAbilityInfo(this);
	if (!AbilityInfos)
	{
		return;
	}
	
	TArray<FGameplayAbilityInfo> AbilityInfoList = AbilityInfos->FindAbilityInfoForListOfTag(AbilitiesToGive, true);
	
	for (FGameplayAbilityInfo const& abilityInfo : AbilityInfoList)
	{
		FGameplayAbilitySpec abilitySpec{abilityInfo.AbilityClass, Level};
		
		// Add input tag if need be
		if (abilityInfo.TypeOfAbilityTag.IsValid())
		{
			if (FGameplayTag const* FoundTag = AbilityInfos->AbilityTypeToInputTagMap.Find(abilityInfo.TypeOfAbilityTag))
			{
				abilitySpec.GetDynamicSpecSourceTags().AddTag(*FoundTag);
			}
			else
			{
				UE_LOG(LogGeoASC, Error, TEXT("Input Tag for ability of type %s not found in map AbilityTypeToInputTagMap (UAbilityInfo)"), *abilityInfo.TypeOfAbilityTag.ToString());
			}
		}
		
		GiveAbility(abilitySpec);
	}
	
	bStartupAbilitiesGiven = true;	
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySystemComponent::AddCharacterStartupAbilities(const int32 Level)
{
	AddCharacterStartupAbilities(StartupAbilityTags, Level);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& inputTag)
{
	if (!inputTag.IsValid())
	{
		return;
	}

	UE_VLOG(this, LogGeoASC, VeryVerbose, TEXT("AbilityInputTag Pressed of INPUT %s"), *inputTag.ToString());

	FScopedAbilityListLock activeScopeLock(*this);
	for (FGameplayAbilitySpec& abilitySpec : GetActivatableAbilities())
	{
		// Only activate ability of given input tag
		if (!abilitySpec.GetDynamicSpecSourceTags().HasTagExact(inputTag))
		{
			continue;
		}

		AbilitySpecInputPressed(abilitySpec);
		if (abilitySpec.IsActive())
		{
			PRAGMA_DISABLE_DEPRECATION_WARNINGS
			// Code from Lyra starter game (if they disable Deprecation warnings, I don't see why not do the same)
			const UGameplayAbility* Instance = abilitySpec.GetPrimaryInstance();
			FPredictionKey originalPredictionKey = Instance
			                                         ? Instance->GetCurrentActivationInfo().GetActivationPredictionKey()
			                                         : abilitySpec.ActivationInfo.GetActivationPredictionKey();
			PRAGMA_ENABLE_DEPRECATION_WARNINGS

			InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, abilitySpec.Handle,
				originalPredictionKey);
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySystemComponent::AbilityInputTagHeld(const FGameplayTag& inputTag)
{
	if (!inputTag.IsValid())
	{
		return;
	}
	FName Name = inputTag.GetTagName();
	if (Name == "InputTag.SpecialSpell")
	{
		UE_LOG(LogTemp, Warning, TEXT("AbilityInputTagHeld of INPUT %s"), *inputTag.ToString());
	}

	FScopedAbilityListLock activeScopeLock(*this);
	for (FGameplayAbilitySpec& abilitySpec : GetActivatableAbilities())
	{
		if (!abilitySpec.GetDynamicSpecSourceTags().HasTagExact(inputTag))
		{
			continue;
		}

		AbilitySpecInputPressed(abilitySpec);
		if (!abilitySpec.IsActive())
		{
			TryActivateAbility(abilitySpec.Handle);
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& inputTag)
{
	if (!inputTag.IsValid())
	{
		return;
	}

	FScopedAbilityListLock activeScopeLock(*this);
	for (FGameplayAbilitySpec& abilitySpec : GetActivatableAbilities())
	{
		if (abilitySpec.GetDynamicSpecSourceTags().HasTagExact(inputTag) && abilitySpec.IsActive())
		{
			AbilitySpecInputReleased(abilitySpec);

			PRAGMA_DISABLE_DEPRECATION_WARNINGS
			// Code from Lyra starter game (if they disable Deprecation warnings, I don't see why not do the same)
			const UGameplayAbility* Instance = abilitySpec.GetPrimaryInstance();
			FPredictionKey originalPredictionKey = Instance
			                                         ? Instance->GetCurrentActivationInfo().GetActivationPredictionKey()
			                                         : abilitySpec.ActivationInfo.GetActivationPredictionKey();
			PRAGMA_ENABLE_DEPRECATION_WARNINGS

			// Needed to use Wait for input release in blueprint
			InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, abilitySpec.Handle,
				originalPredictionKey);
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySystemComponent::ApplyEffectToSelf(TSubclassOf<UGameplayEffect> GameplayEffectClass, int32 Level /*= 1*/)
{
	FGameplayEffectContextHandle EffectContextHandle = MakeEffectContext();
	EffectContextHandle.AddSourceObject(this);

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingSpec(GameplayEffectClass, Level, EffectContextHandle);

	if (SpecHandle.IsValid())
	{
		FPredictionKey PredictionKey;
		ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), this, PredictionKey);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySystemComponent::InitializeDefaultAttributes(int32 Level /*= 1*/)
{
	if (!IsValid(DefaultAttributes))
	{
		UE_LOG(LogGeoTrinity, Error,
			TEXT("%s() Missing DefaultAttributes for %s. Please fill in the Owner's Blueprint."), *FString(__FUNCTION__),
			*GetName());
		return;
	}

	ApplyEffectToSelf(DefaultAttributes, Level);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySystemComponent::BindAttributeCallbacks()
{
	GetGameplayAttributeValueChangeDelegate(UGeoAttributeSetBase::GetHealthAttribute())
		.AddWeakLambda(this,
			[this](const FOnAttributeChangeData& Data)
			{
				OnHealthChanged.Broadcast(Data.NewValue);
			});
	GetGameplayAttributeValueChangeDelegate(UGeoAttributeSetBase::GetMaxHealthAttribute())
		.AddWeakLambda(this,
			[this](const FOnAttributeChangeData& Data)
			{
				OnMaxHealthChanged.Broadcast(Data.NewValue);
			});
}