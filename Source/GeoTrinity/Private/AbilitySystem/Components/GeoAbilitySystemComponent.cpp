// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
#include "AbilitySystem/Abilities/Base/PatternAbility.h"
#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
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

/** The ability catalog, or null after one ensure naming Caller. Every reader of the catalog goes through here. */
UAbilityInfo* GetCheckedAbilityInfo(ANSICHAR const* const Caller)
{
	UAbilityInfo* AbilityInfos = UGeoAbilitySystemLibrary::GetAbilityInfo();
	ensureMsgf(AbilityInfos, TEXT("%hs: AbilityInfo data asset is not loaded"), Caller);
	return AbilityInfos;
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

	UAbilityInfo const* AbilityInfos = GetCheckedAbilityInfo(__FUNCTION__);
	if (!AbilityInfos)
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

	UAbilityInfo const* AbilityInfos = GetCheckedAbilityInfo(__FUNCTION__);
	if (!AbilityInfos)
	{
		return;
	}

	for (FGameplayAbilityInfo const& AbilityInfo : AbilityInfos->GetAllAbilityInfos())
	{
		if (!AbilityInfo.AbilityClass)
		{
			continue;
		}

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
	float const FireDelay = RemoteFire.AbilityCDO->GetFireDelay();
	if (FireDelay <= 0.f)
	{
		RemoteFireShot(RemoteFireTag);
		return;
	}

	// The tag spent a ping reaching us, so only the first shot is that much closer: subtracting it from the rate too
	// would make a looping replay fire faster the worse the local connection is.
	GetWorld()->GetTimerManager().SetTimer(
		RemoteFire.ShotTimer, FTimerDelegate::CreateUObject(this, &ThisClass::RemoteFireShot, RemoteFireTag), FireDelay,
		RemoteFire.AbilityCDO->IsRemoteFireLooping(),
		FMath::Max(FireDelay - GeoLib::GetOnWayPingSec(GetWorld()), 0.f));
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
void UGeoAbilitySystemComponent::GiveAbilitySpec(TSubclassOf<UGameplayAbility> const AbilityClass,
												 FGameplayTag const InputTag)
{
	FGameplayAbilitySpec AbilitySpec{AbilityClass, CombatLevel};
	if (InputTag.IsValid())
	{
		AbilitySpec.GetDynamicSpecSourceTags().AddTag(InputTag);
	}
	GiveAbility(AbilitySpec);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySystemComponent::GiveStartupAbilities()
{
	UAbilityInfo const* AbilityInfos = GetCheckedAbilityInfo(__FUNCTION__);
	if (!AbilityInfos)
	{
		return;
	}

	for (FGameplayAbilityInfo const& AbilityInfo : AbilityInfos->FindAbilityInfoForListOfTag(StartupAbilityTags, true))
	{
		GiveAbilitySpec(AbilityInfo.AbilityClass, FGameplayTag());
	}

	bStartupAbilitiesGiven = true;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySystemComponent::GiveStartupAbilities(EPlayerClass const PlayerClass)
{
	UAbilityInfo const* AbilityInfos = GetCheckedAbilityInfo(__FUNCTION__);
	if (!AbilityInfos)
	{
		return;
	}

	for (FPlayersGameplayAbilityInfo const& AbilityInfo : AbilityInfos->GetAbilitiesForClass(PlayerClass))
	{
		if (!AbilityInfo.bGiveAtStartup)
		{
			continue;
		}

		bool const bHasInput = AbilityInfo.InputAction != nullptr;
		ensureMsgf(!bHasInput || AbilityInfo.InputTag.IsValid(),
				   TEXT("Ability %s has an InputAction but no InputTag — fill InputTag in DA_AbilityInfo"),
				   *AbilityInfo.AbilityClass->GetName());

		GiveAbilitySpec(AbilityInfo.AbilityClass, bHasInput ? AbilityInfo.InputTag : FGameplayTag());
	}

	bStartupAbilitiesGiven = true;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySystemComponent::ClearPlayerClassAbilities()
{
	UAbilityInfo const* AbilityInfos = GetCheckedAbilityInfo(__FUNCTION__);
	if (!AbilityInfos)
	{
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
FPredictionKey UGeoAbilitySystemComponent::GetActivationPredictionKey(FGameplayAbilitySpec const& AbilitySpec)
{
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	// Code from Lyra starter game (if they disable Deprecation warnings, I don't see why not do the same)
	UGameplayAbility const* Instance = AbilitySpec.GetPrimaryInstance();
	return Instance ? Instance->GetCurrentActivationInfo().GetActivationPredictionKey()
					: AbilitySpec.ActivationInfo.GetActivationPredictionKey();
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySystemComponent::ActivateAbilitiesForInput(FGameplayTag const& InputTag, bool const bFreshPress)
{
	FScopedAbilityListLock ActiveScopeLock(*this);
	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		UGeoGameplayAbility const* GeoAbility = Cast<UGeoGameplayAbility>(AbilitySpec.Ability);

		// An ability can name a second input that fires it: that press releases it, its own input still does too.
		if (bFreshPress && GeoAbility && GeoAbility->GetAlternateReleaseInputTag() == InputTag && AbilitySpec.IsActive())
		{
			ReleaseAbilitySpec(AbilitySpec);
		}

		if (!AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			continue;
		}

		AbilitySpecInputPressed(AbilitySpec);

		// Fresh-press-only abilities are excluded from the per-frame Held activation, so only the press activates them.
		bool const bFreshPressOnly = GeoAbility && GeoAbility->bActivateOnFreshPressOnly;
		if (!AbilitySpec.IsActive() && bFreshPressOnly == bFreshPress)
		{
			TryActivateAbilityWithTargetData(AbilitySpec.Handle, GeoASLib::GetAbilityTagFromSpec(AbilitySpec));
		}

		if (bFreshPress && AbilitySpec.IsActive())
		{
			InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, AbilitySpec.Handle,
								  GetActivationPredictionKey(AbilitySpec));
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySystemComponent::AbilityInputTagPressed(FGameplayTag const& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	UE_VLOG(this, LogGeoASC, VeryVerbose, TEXT("AbilityInputTag Pressed of INPUT %s"), *InputTag.ToString());
	ActivateAbilitiesForInput(InputTag, /*bFreshPress=*/true);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySystemComponent::AbilityInputTagHeld(FGameplayTag const& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	ActivateAbilitiesForInput(InputTag, /*bFreshPress=*/false);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySystemComponent::AbilityInputTagReleased(FGameplayTag const& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	FScopedAbilityListLock ActiveScopeLock(*this);
	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag) && AbilitySpec.IsActive())
		{
			ReleaseAbilitySpec(AbilitySpec);
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySystemComponent::ReleaseAbilitySpec(FGameplayAbilitySpec& AbilitySpec)
{
	AbilitySpecInputReleased(AbilitySpec);

	// Needed to use Wait for input release in blueprint
	InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, AbilitySpec.Handle,
						  GetActivationPredictionKey(AbilitySpec));
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySystemComponent::ApplyEffectToSelf(TSubclassOf<UGameplayEffect> GameplayEffectClass)
{
	FGameplayEffectContextHandle EffectContextHandle = MakeEffectContext();
	EffectContextHandle.AddSourceObject(this);

	FGameplayEffectSpecHandle const SpecHandle = MakeOutgoingSpec(GameplayEffectClass, CombatLevel, EffectContextHandle);
	if (!ensureMsgf(SpecHandle.IsValid(), TEXT("%hs: SpecHandle is invalid"), __FUNCTION__))
	{
		return;
	}

	ensureMsgf(IsMaxHealthModifiedFirst(*SpecHandle.Data->Def),
			   TEXT("%s modifies Health or Shield before MaxHealth, so they land clamped to 0. Move the MaxHealth "
					"modifier to the top of the effect's Modifiers array."),
			   *GameplayEffectClass->GetName());

	ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), this);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySystemComponent::InitializeDefaultAttributes()
{
	if (!IsValid(DefaultAttributes))
	{
		UE_LOG(LogGeoTrinity, Error,
			   TEXT("%s() Missing DefaultAttributes for %s. Please fill in the Owner's Blueprint."),
			   *FString(__FUNCTION__), *GetName());
		return;
	}

	ApplyEffectToSelf(DefaultAttributes);
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

	if (!ensureMsgf(IsValid(GetOwnerActor()), TEXT("%hs: invalid OwnerActor"), __FUNCTION__))
	{
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

		if (!ensureMsgf(IsValid(CDO), TEXT("%hs: no ability CDO found for tag %s"), __FUNCTION__,
						*AbilityTag.ToString()))
		{
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

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySystemComponent::ResetCooldowns()
{
	FGameplayTag const CooldownRoot = FGeoGameplayTags::Get().Ability_Cooldown;

	if (GeoLib::IsServer(this))
	{
		FGameplayEffectQuery Query;
		Query.OwningTagQuery = FGameplayTagQuery::MakeQuery_MatchAnyTags(CooldownRoot.GetSingleTagContainer());
		RemoveActiveEffects(Query);
	}

	// Iterated over a copy, so clearing a count as we go is safe. On the server the removal above already took every
	// count to zero and this finds nothing; on a client it is the whole reset, since the effect removal replicates down
	// without ever lowering a tag the client raised itself.
	FGameplayTagContainer OwnedTags;
	GetOwnedGameplayTags(OwnedTags);
	for (FGameplayTag const& Tag : OwnedTags)
	{
		if (Tag.MatchesTag(CooldownRoot))
		{
			SetTagMapCount(Tag, 0);
		}
	}
}
