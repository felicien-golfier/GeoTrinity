// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
#include "AbilitySystem/Abilities/Base/PatternAbility.h"
#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Types/GeoAscTypes.h"
#include "Characters/Component/GeoCharacterMovementComponent.h"
#include "Characters/GeoCharacter.h"
#include "Characters/PlayerClassTypes.h"
#include "GameplayEffect.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Tool/UGeoGameplayLibrary.h"

namespace
{
// GAS executes an effect's modifiers strictly in array order, and UGeoAttributeSetBase::PreAttributeChange clamps
// Health and Shield against MaxHealth. A modifier listed before MaxHealth therefore lands clamped against a MaxHealth
// of 0 — and the clamp hits the current value only, so raising MaxHealth afterwards never brings it back.
bool IsMaxHealthModifiedFirst(UGameplayEffect const& Effect)
{
	bool bSeenClampedAttribute = false;
	for (FGameplayModifierInfo const& Modifier : Effect.Modifiers)
	{
		if (Modifier.Attribute == UGeoAttributeSetBase::GetMaxHealthAttribute())
		{
			return !bSeenClampedAttribute;
		}
		bSeenClampedAttribute |= Modifier.Attribute == UGeoAttributeSetBase::GetHealthAttribute()
			|| Modifier.Attribute == UGeoAttributeSetBase::GetShieldAttribute();
	}
	return true;
}
} // namespace

void UGeoAbilitySystemComponent::InitializeComponent()
{
	Super::InitializeComponent();

	UWorld const* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		// Editor preview worlds, editor world, etc.
		return;
	}

	UAbilityInfo* AbilityInfos = UGeoAbilitySystemLibrary::GetAbilityInfo();
	if (!ensureMsgf(AbilityInfos, TEXT("UGeoAbilitySystemComponent: AbilityInfo data asset is not loaded")))
	{
		return;
	}

	for (FGameplayAbilityInfo const& AbilityInfo : AbilityInfos->GetAllAbilityInfos())
	{
		if (!AbilityInfo.AbilityClass)
		{
			continue;
		}

		if (AbilityInfo.AbilityClass->IsChildOf(UPatternAbility::StaticClass())
			&& StartupAbilityTags.Contains(AbilityInfo.AbilityTag))
		{
			UPatternAbility* PatternAbilityCDO =
				CastChecked<UPatternAbility>(AbilityInfo.AbilityClass->GetDefaultObject());
			UPattern* Pattern;
			if (!FindPatternByClass(PatternAbilityCDO->GetPatternClass(), Pattern))
			{
				CreatePatternInstance(PatternAbilityCDO->GetPatternClass(), AbilityInfo.AbilityTag);
			}
		}
	}
}

void UGeoAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);

	BindAttributeCallbacks();

	if (!GeoLib::IsServer(this))
	{
		BindRemoteFireTags();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySystemComponent::BindRemoteFireTags()
{
	if (!RemoteFires.IsEmpty())
	{
		return;
	}

	for (FGameplayAbilityInfo const& AbilityInfo : UGeoAbilitySystemLibrary::GetAbilityInfo()->GetAllAbilityInfos())
	{
		UGeoGameplayAbility* AbilityCDO = Cast<UGeoGameplayAbility>(AbilityInfo.AbilityClass->GetDefaultObject());
		if (!AbilityCDO || !AbilityCDO->RemoteFireTag.IsValid())
		{
			continue;
		}

		if (!ensureMsgf(!RemoteFires.Contains(AbilityCDO->RemoteFireTag),
						TEXT("%s reuses RemoteFireTag %s: each ability needs its own, or both replay on every shot."),
						*AbilityCDO->GetName(), *AbilityCDO->RemoteFireTag.ToString()))
		{
			continue;
		}

		RemoteFires.Add(AbilityCDO->RemoteFireTag, FRemoteFire{AbilityCDO});
		RegisterGameplayTagEvent(AbilityCDO->RemoteFireTag, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &ThisClass::OnRemoteFireTagChanged);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySystemComponent::OnRemoteFireTagChanged(FGameplayTag const RemoteFireTag, int32 const NewCount)
{
	FRemoteFire& RemoteFire = RemoteFires.FindChecked(RemoteFireTag);
	GetWorld()->GetTimerManager().ClearTimer(RemoteFire.ShotTimer);

	// Same predicate UGeoGameplayAbility::Fire uses to spawn the real projectile, so the two can never both run.
	// Checked here rather than at the binding site: the pawn's controller can still be unset when the ASC is
	// initialised, and InitAbilityActorInfo runs again once it is.
	if (NewCount <= 0 || AbilityActorInfo->IsLocallyControlledPlayer())
	{
		return;
	}

	// Mirrors ScheduleFireTrigger: the shot lands one fire delay after activation, or right away when there is none.
	float const FireDelay = RemoteFire.AbilityCDO->GetFireDelay() - GeoLib::GetOnWayPingSec(GetWorld());
	if (FireDelay <= 0.f)
	{
		RemoteFireShot(RemoteFireTag);
		return;
	}

	GetWorld()->GetTimerManager().SetTimer(
		RemoteFire.ShotTimer, FTimerDelegate::CreateUObject(this, &ThisClass::RemoteFireShot, RemoteFireTag), FireDelay,
		RemoteFire.AbilityCDO->IsRemoteFireLooping());
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySystemComponent::RemoteFireShot(FGameplayTag const RemoteFireTag)
{
	AActor* Avatar = GetAvatarActor();
	if (!IsValid(Avatar))
	{
		return;
	}

	UGeoGameplayAbility const* AbilityCDO = RemoteFires.FindChecked(RemoteFireTag).AbilityCDO;

	// Nothing advanced the counter here: the ally's ability never instances on this machine. The montage does
	// replicate, so the section it is playing tells which fire socket the shot leaves from.
	FireSectionIndices.Add(AbilityCDO->GetAbilityTag(), AbilityCDO->GetPlayingFireSectionIndex(Avatar));

	AbilityCDO->RemoteFireShot(Avatar, this);
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
		UGeoGameplayAbility const* GeoAbility = Cast<UGeoGameplayAbility>(abilitySpec.Ability);

		// An ability can name a second input that fires it: this press releases it, its own input still does too.
		if (GeoAbility && GeoAbility->GetAlternateReleaseInputTag() == inputTag && abilitySpec.IsActive())
		{
			ReleaseAbilitySpec(abilitySpec);
		}

		// Only activate ability of given input tag
		if (!abilitySpec.GetDynamicSpecSourceTags().HasTagExact(inputTag))
		{
			continue;
		}

		AbilitySpecInputPressed(abilitySpec);

		// Fresh-press-only abilities are excluded from the per-frame Held activation, so the press activates them.
		if (!abilitySpec.IsActive() && GeoAbility && GeoAbility->bActivateOnFreshPressOnly)
		{
			TryActivateAbilityWithTargetData(abilitySpec.Handle, GeoASLib::GetAbilityTagFromSpec(abilitySpec));
		}

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

		UGeoGameplayAbility const* GeoAbility = Cast<UGeoGameplayAbility>(abilitySpec.Ability);
		if (!abilitySpec.IsActive() && !(GeoAbility && GeoAbility->bActivateOnFreshPressOnly))
		{
			TryActivateAbilityWithTargetData(abilitySpec.Handle, GeoASLib::GetAbilityTagFromSpec(abilitySpec));
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
			ReleaseAbilitySpec(abilitySpec);
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySystemComponent::ReleaseAbilitySpec(FGameplayAbilitySpec& AbilitySpec)
{
	AbilitySpecInputReleased(AbilitySpec);

	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	// Code from Lyra starter game (if they disable Deprecation warnings, I don't see why not do the same)
	UGameplayAbility const* Instance = AbilitySpec.GetPrimaryInstance();
	FPredictionKey originalPredictionKey = Instance ? Instance->GetCurrentActivationInfo().GetActivationPredictionKey()
													: AbilitySpec.ActivationInfo.GetActivationPredictionKey();
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	// Needed to use Wait for input release in blueprint
	InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, AbilitySpec.Handle, originalPredictionKey);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySystemComponent::ApplyEffectToSelf(TSubclassOf<UGameplayEffect> GameplayEffectClass,
												   int32 Level /*= 1*/)
{
	FGameplayEffectContextHandle EffectContextHandle = MakeEffectContext();
	EffectContextHandle.AddSourceObject(this);

	FGameplayEffectSpecHandle const SpecHandle = MakeOutgoingSpec(GameplayEffectClass, Level, EffectContextHandle);
	if (!SpecHandle.IsValid())
	{
		ensureMsgf(false, TEXT("ApplyEffectToSelf: SpecHandle is invalid!"));
		return;
	}

	ensureMsgf(IsMaxHealthModifiedFirst(*SpecHandle.Data->Def),
			   TEXT("%s modifies Health or Shield before MaxHealth, so they land clamped to 0. Move the MaxHealth "
					"modifier to the top of the effect's Modifiers array."),
			   *GameplayEffectClass->GetName());

	ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), this);
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
	if (bAttributeCallbacksBound)
	{
		return;
	}
	bAttributeCallbacksBound = true;

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

	if (!IsValid(GetOwnerActor()))
	{
		ensureMsgf(IsValid(GetOwnerActor()), TEXT("CreatePatternInstance: Invalid OwnerActor"));
		return nullptr;
	}

	UPattern* PatternInstance = NewObject<UPattern>(this, PatternClass);

	PatternInstance->OnCreate(AbilityTag, *GetOwnerActor());
	Patterns.Add(PatternInstance);

	return PatternInstance;
}

void UGeoAbilitySystemComponent::StopAllActivePatterns()
{
	for (UPattern* Pattern : Patterns)
	{
		if (IsValid(Pattern) && Pattern->IsPatternActive())
		{
			Pattern->EndPattern(true);
		}
	}
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

void UGeoAbilitySystemComponent::PatternStartMulticast_Implementation(FAbilityPayload Payload, UClass* PatternClass,
																	  TInstancedStruct<FPatternData> PatternData)
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

	Pattern->InitPattern(Payload, PatternData);
}

int32& UGeoAbilitySystemComponent::GetFireSectionIndex(FGameplayTag const& AbilityTag)
{
	return FireSectionIndices.FindOrAdd(AbilityTag, -1);
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoAbilitySystemComponent::TryActivateAbilityWithTargetData(FGameplayAbilitySpecHandle Handle,
																  FGameplayTag const AbilityTag)
{
	// Build event data with avatar's current orientation
	FGameplayEventData EventData;

	if (AActor* Avatar = GetAvatarActor())
	{
		UGeoGameplayAbility const* CDO = GeoASLib::GetAbilityCDO(AbilityTag);

		if (!IsValid(CDO))
		{
			ensureMsgf(CDO, TEXT("TryActivateAbilityWithTargetData: no ability CDO found for Tag %s"),
					   *AbilityTag.ToString());
			return false;
		}

		FGameplayAbilityTargetDataHandle TargetDataHandle;
		int const Seed = CDO->GetNewSeed();
		FGeoAbilityTargetData* TargetData =
			new FGeoAbilityTargetData(CDO->GetFireOrigin2D(Avatar, this, Seed), CDO->GetFireYaw(Avatar, Seed),
									  CDO->GetStartTime(GetWorld()), Seed);
		TargetDataHandle.Add(TargetData);
		EventData.TargetData = TargetDataHandle;
		EventData.Instigator = Avatar;
	}

	// Use InternalTryActivateAbility which accepts TriggerEventData
	// This sends the event data with the activation RPC (single packet)
	return InternalTryActivateAbility(Handle, FPredictionKey(), nullptr, nullptr, &EventData);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySystemComponent::ReactivatePassiveAbilities()
{
	FScopedAbilityListLock activeScopeLock(*this);
	for (FGameplayAbilitySpec& abilitySpec : GetActivatableAbilities())
	{
		UGeoGameplayAbility const* AbilityCDO = Cast<UGeoGameplayAbility>(abilitySpec.Ability);
		if (AbilityCDO && AbilityCDO->IsPassive() && !abilitySpec.IsActive())
		{
			TryActivateAbility(abilitySpec.Handle, false);
		}
	}
}
