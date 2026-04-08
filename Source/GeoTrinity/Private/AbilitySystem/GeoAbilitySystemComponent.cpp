// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/GeoAbilitySystemComponent.h"

#include "AbilitySystem/Abilities/PatternAbility.h"
#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "AbilitySystem/GeoAscTypes.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Characters/GeoCharacter.h"
#include "Characters/PlayerClassTypes.h"
#include "GeoMovementComponent.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Tool/UGeoGameplayLibrary.h"

void UGeoAbilitySystemComponent::InitializeComponent()
{
	Super::InitializeComponent();

	UWorld const* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		// Editor preview worlds, editor world, etc.
		return;
	}

	for (auto const AbilityInfo : UGeoAbilitySystemLibrary::GetAbilityInfo()->GetAllAbilityInfos())
	{
		if (AbilityInfo->AbilityClass->IsChildOf(UPatternAbility::StaticClass())
			&& StartupAbilityTags.Contains(AbilityInfo->AbilityTag))
		{
			UPatternAbility* PatternAbilityCDO =
				CastChecked<UPatternAbility>(AbilityInfo->AbilityClass->GetDefaultObject());
			UPattern* Pattern;
			if (!FindPatternByClass(PatternAbilityCDO->GetPatternClass(), Pattern))
			{
				CreatePatternInstance(PatternAbilityCDO->GetPatternClass(), AbilityInfo->AbilityTag);
			}
		}
	}
}

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
void UGeoAbilitySystemComponent::GiveStartupAbilities(TArray<FGameplayTag> const& AbilitiesToGive, int32 const Level)
{
	UAbilityInfo* AbilityInfos = UGeoAbilitySystemLibrary::GetAbilityInfo(this);
	if (!AbilityInfos)
	{
		ensureMsgf(AbilityInfos, TEXT("GiveStartupAbilities: AbilityInfo not set!"));
		return;
	}

	TArray<FGameplayAbilityInfo> AbilityInfoList = AbilityInfos->FindAbilityInfoForListOfTag(AbilitiesToGive, true);

	for (FGameplayAbilityInfo const& AbilityInfo : AbilityInfoList)
	{
		FGameplayAbilitySpec abilitySpec{AbilityInfo.AbilityClass, Level};
		GiveAbility(abilitySpec);
	}

	bStartupAbilitiesGiven = true;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySystemComponent::GiveStartupAbilities(int32 const Level)
{
	GiveStartupAbilities(StartupAbilityTags, Level);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySystemComponent::GiveStartupAbilities(EPlayerClass const PlayerClass, int32 const Level)
{
	UAbilityInfo* AbilityInfos = UGeoAbilitySystemLibrary::GetAbilityInfo(this);
	if (!AbilityInfos)
	{
		ensureMsgf(AbilityInfos, TEXT("GiveStartupAbilities: AbilityInfo not set!"));
		return;
	}

	for (FPlayersGameplayAbilityInfo const& AbilityInfo : AbilityInfos->GetAbilitiesForClass(PlayerClass))
	{
		if (!AbilityInfo.bGiveAtStartup)
		{
			continue;
		}

		FGameplayAbilitySpec AbilitySpec{AbilityInfo.AbilityClass, Level};

		if (AbilityInfo.InputAction && !AbilityInfo.InputTag.IsValid())
		{
			ensureMsgf(AbilityInfo.InputTag.IsValid(),
					   TEXT("Ability %s has an InputAction but no InputTag — fill InputTag in DA_AbilityInfo"),
					   *AbilityInfo.AbilityClass->GetName());
		}
		else if (AbilityInfo.InputAction)
		{
			AbilitySpec.GetDynamicSpecSourceTags().AddTag(AbilityInfo.InputTag);
		}

		GiveAbility(AbilitySpec);
	}

	bStartupAbilitiesGiven = true;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySystemComponent::ClearPlayerClassAbilities()
{
	UAbilityInfo* AbilityInfos = UGeoAbilitySystemLibrary::GetAbilityInfo(this);
	if (!AbilityInfos)
	{
		ensureMsgf(AbilityInfos, TEXT("ClearPlayerClassAbilities: AbilityInfo not set!"));
		return;
	}

	TArray<FGameplayAbilitySpecHandle> AbilitiesToClear;
	{
		FScopedAbilityListLock ScopeLock(*this);
		for (FGameplayAbilitySpec const& AbilitySpec : GetActivatableAbilities())
		{
			if (!AbilitySpec.Ability)
			{
				continue;
			}
			for (FPlayersGameplayAbilityInfo const& Info : AbilityInfos->GetAllPlayersAbilityInfos())
			{
				if (AbilitySpec.Ability->GetClass() == Info.AbilityClass)
				{
					AbilitiesToClear.Add(AbilitySpec.Handle);
					break;
				}
			}
		}
	}

	for (FGameplayAbilitySpecHandle const& Handle : AbilitiesToClear)
	{
		ClearAbility(Handle);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySystemComponent::AbilityInputTagPressed(FGameplayTag const& inputTag)
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
			UGameplayAbility const* Instance = abilitySpec.GetPrimaryInstance();
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
void UGeoAbilitySystemComponent::AbilityInputTagHeld(FGameplayTag const& inputTag)
{
	if (!inputTag.IsValid())
	{
		return;
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
			TryActivateAbilityWithTargetData(abilitySpec.Handle);
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySystemComponent::AbilityInputTagReleased(FGameplayTag const& inputTag)
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
			UGameplayAbility const* Instance = abilitySpec.GetPrimaryInstance();
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
void UGeoAbilitySystemComponent::ApplyEffectToSelf(TSubclassOf<UGameplayEffect> GameplayEffectClass,
												   int32 Level /*= 1*/)
{
	FGameplayEffectContextHandle EffectContextHandle = MakeEffectContext();
	EffectContextHandle.AddSourceObject(this);

	FGameplayEffectSpecHandle const SpecHandle = MakeOutgoingSpec(GameplayEffectClass, Level, EffectContextHandle);

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
			   TEXT("%s() Missing DefaultAttributes for %s. Please fill in the Owner's Blueprint."),
			   *FString(__FUNCTION__), *GetName());
		return;
	}

	ApplyEffectToSelf(DefaultAttributes, Level);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySystemComponent::BindAttributeCallbacks()
{
	GetGameplayAttributeValueChangeDelegate(UGeoAttributeSetBase::GetHealthAttribute())
		.AddWeakLambda(this,
					   [this](FOnAttributeChangeData const& Data)
					   {
						   OnHealthChanged.Broadcast(Data.NewValue);
					   });
	GetGameplayAttributeValueChangeDelegate(UGeoAttributeSetBase::GetMaxHealthAttribute())
		.AddWeakLambda(this,
					   [this](FOnAttributeChangeData const& Data)
					   {
						   OnMaxHealthChanged.Broadcast(Data.NewValue);
					   });

	GetGameplayAttributeValueChangeDelegate(UCharacterAttributeSet::GetMovementSpeedMultiplierAttribute())
		.AddWeakLambda(this,
					   [this](FOnAttributeChangeData const& Data)
					   {
						   AGeoCharacter* GeoCharacter = Cast<AGeoCharacter>(GetAvatarActor());
						   if (GeoCharacter)
						   {
							   GeoCharacter->GetGeoMovementComponent()->ApplySpeedMultiplier(Data.NewValue);
						   }
					   });
}

UPattern* UGeoAbilitySystemComponent::CreatePatternInstance(UClass const* PatternClass, FGameplayTag AbilityTag)
{
	if (!PatternClass)
	{
		UE_LOG(LogGeoTrinity, Error, TEXT("CreatePatternInstance: Invalid PatternClass"));
		return nullptr;
	}

	UPattern* PatternInstance = NewObject<UPattern>(this, PatternClass);
	PatternInstance->OnCreate(AbilityTag);
	Patterns.Add(PatternInstance);

	return PatternInstance;
}

bool UGeoAbilitySystemComponent::FindPatternByClass(UClass* PatternClass, UPattern*& Pattern)
{
	UPattern** FoundPattern = Patterns.FindByPredicate(
		[PatternClass](UPattern const* Candidate)
		{
			return IsValid(Candidate) && Candidate->IsA(PatternClass);
		});

	if (!FoundPattern)
	{
		return false;
	}

	Pattern = *FoundPattern;
	return true;
}

void UGeoAbilitySystemComponent::PatternStartMulticast_Implementation(FAbilityPayload Payload, UClass* PatternClass)
{
	checkf(PatternClass, TEXT("PatternStartMulticast: Invalid PatternClass"));

	UPattern* Pattern;
	if (!FindPatternByClass(PatternClass, Pattern))
	{
		UE_LOG(LogGeoASC, Warning,
			   TEXT("PatternStartMulticast: Pattern instance of class %s not found! It should have been created."),
			   *PatternClass->GetName());

		// Fallback to maintain functionality if OnGiveAbility wasn't called for some reason
		Pattern = CreatePatternInstance(PatternClass, Payload.AbilityTag);
	}

	Pattern->InitPattern(Payload);
}

int32& UGeoAbilitySystemComponent::GetFireSectionIndex(FGameplayTag const& AbilityTag)
{
	return FireSectionIndices.FindOrAdd(AbilityTag, -1);
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoAbilitySystemComponent::TryActivateAbilityWithTargetData(FGameplayAbilitySpecHandle Handle)
{
	// Build event data with avatar's current orientation
	FGameplayEventData EventData;

	if (AActor* Avatar = GetAvatarActor())
	{
		FGameplayAbilityTargetDataHandle TargetDataHandle;
		FGeoAbilityTargetData* TargetData =
			new FGeoAbilityTargetData(FVector2D(Avatar->GetActorLocation()), Avatar->GetActorRotation().Yaw,
									  GeoLib::GetServerTime(GetWorld(), true), FMath::Rand32());
		TargetDataHandle.Add(TargetData);
		EventData.TargetData = TargetDataHandle;
		EventData.Instigator = Avatar;
	}

	// Use InternalTryActivateAbility which accepts TriggerEventData
	// This sends the event data with the activation RPC (single packet)
	return InternalTryActivateAbility(Handle, FPredictionKey(), nullptr, nullptr, &EventData);
}
