// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Common/GeoDeployAbility.h"

#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystemComponent.h"
#include "Actor/Projectile/DeployableSpawner/DeployableSpawnerProjectile.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "Settings/GameDataSettings.h"
#include "Tool/UGeoGameplayLibrary.h"

UGeoDeployAbility::UGeoDeployAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	FireMode = EFireMode::ChargeForFireDelay;
	CommitBehaviour = ECommitBehaviour::DoNotAutoCommit;
	bOverrideSpeed = true;
	ProjectileSpeed = 2000.f;
	bActivateOnFreshPressOnly = true;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoDeployAbility::OnGiveAbility(FGameplayAbilityActorInfo const* ActorInfo, FGameplayAbilitySpec const& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	CurrentStacks = MaxStacks;

	FGameplayTagContainer const* CooldownTags = GetCooldownTags();
	ensureMsgf(CooldownTags && !CooldownTags->IsEmpty(),
			   TEXT("GeoDeployAbility '%s': cooldown GE grants no tags; stack refill cannot be tracked."), *GetName());
	if (CooldownTags && !CooldownTags->IsEmpty())
	{
		CooldownTagDelegateHandle =
			ActorInfo->AbilitySystemComponent
				->RegisterGameplayTagEvent(CooldownTags->First(), EGameplayTagEventType::NewOrRemoved)
				.AddUObject(this, &ThisClass::OnCooldownTagChanged);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoDeployAbility::OnRemoveAbility(FGameplayAbilityActorInfo const* ActorInfo, FGameplayAbilitySpec const& Spec)
{
	FGameplayTagContainer const* CooldownTags = GetCooldownTags();
	if (CooldownTagDelegateHandle.IsValid() && CooldownTags && !CooldownTags->IsEmpty())
	{
		ActorInfo->AbilitySystemComponent->UnregisterGameplayTagEvent(CooldownTagDelegateHandle, CooldownTags->First(),
																	  EGameplayTagEventType::NewOrRemoved);
		CooldownTagDelegateHandle.Reset();
	}

	Super::OnRemoveAbility(ActorInfo, Spec);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoDeployAbility::ActivateAbility(FGameplayAbilitySpecHandle const Handle,
										FGameplayAbilityActorInfo const* ActorInfo,
										FGameplayAbilityActivationInfo const ActivationInfo,
										FGameplayEventData const* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// Spend a stack only once the activation has actually taken — Super calls EndAbility (clearing IsActive) if it
	// bails on cost. When the shared refill clock is not already ticking, start it so a spend from full begins
	// regenerating immediately.
	if (!IsActive())
	{
		return;
	}

	--CurrentStacks;
	if (GetCooldownTimeRemaining(ActorInfo) <= 0.f)
	{
		CommitAbilityCooldown(Handle, ActorInfo, ActivationInfo, true);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoDeployAbility::CanActivateAbility(FGameplayAbilitySpecHandle const Handle,
										   FGameplayAbilityActorInfo const* ActorInfo,
										   FGameplayTagContainer const* SourceTags,
										   FGameplayTagContainer const* TargetTags,
										   FGameplayTagContainer* OptionalRelevantTags) const
{
	return CurrentStacks > 0
		&& Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoDeployAbility::CheckCooldown(FGameplayAbilitySpecHandle const /*Handle*/,
									  FGameplayAbilityActorInfo const* /*ActorInfo*/,
									  FGameplayTagContainer* /*OptionalRelevantTags*/) const
{
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoDeployAbility::OnCooldownTagChanged(FGameplayTag const /*CooldownTag*/, int32 const NewCount)
{
	// Fires on both the added edge (spend, count 1) and the removed edge (expiry, count 0). The removed edge reaches
	// every machine: the client through its predicted cooldown expiring and through each replicated server-armed
	// cooldown, the server through the authoritative one. Only that edge refills a stack. Re-arming the next cycle is
	// authority-only so the client never applies a non-predicted local cooldown GE (which would double up against the
	// replicated one) — the client's tag re-arms when the server's next cooldown replicates in.
	if (NewCount != 0 || CurrentStacks >= MaxStacks)
	{
		return;
	}

	++CurrentStacks;
	if (CurrentStacks < MaxStacks && GeoLib::IsServer(GetWorld()))
	{
		CommitAbilityCooldown(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true);
	}

	FGameplayTag const CueTag = GetDefault<UGameDataSettings>()->GenericGameplayCueSoundTag;
	if (IsLocallyControlled() && CueTag.IsValid())
	{
		static FGameplayTag const SoundTag =
			FGameplayTag::RequestGameplayTag(FName("Event.Sound.DeployableAvailable"));

		FGameplayCueParameters CueParams;
		CueParams.Instigator = GetAvatarActorFromActorInfo();
		CueParams.AggregatedSourceTags.AddTag(SoundTag);

		// Local-only: each client fires the cue itself off its own predicted/replicated tag edge, so no multicast.
		GetAbilitySystemComponentFromActorInfo()->InvokeGameplayCueEvent(CueTag, EGameplayCueEvent::Executed,
																		 CueParams);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
FGeoAbilityTargetData UGeoDeployAbility::GetUpdatedTargetData()
{
	UGameDataSettings const* GameDataSettings = GetDefault<UGameDataSettings>();
	float PendingDeployDistance =
		FMath::Lerp(GameDataSettings->MinDeployDistance, GameDataSettings->MaxDeployDistance, GetChargeRatio());
	// Encode deploy distance as integer cm in Seed so the server receives it
	StoredPayload.Seed = FMath::RoundToInt(PendingDeployDistance);
	return Super::GetUpdatedTargetData();
}


// ---------------------------------------------------------------------------------------------------------------------
void UGeoDeployAbility::SpawnProjectile(FTransform const& SpawnTransform, float const SpawnServerTime) const
{
	checkf(ProjectileClass, TEXT("No ProjectileClass set on GeoDeployAbility!"));

	FPredictionKey PredictionKey;
	EGameplayAbilityActivationMode::Type const ActivationMode = GetCurrentActivationInfo().ActivationMode;
	if (ActivationMode == EGameplayAbilityActivationMode::Predicting
		|| ActivationMode == EGameplayAbilityActivationMode::Confirmed
		|| ActivationMode == EGameplayAbilityActivationMode::Authority)
	{
		PredictionKey = GetCurrentActivationInfo().GetActivationPredictionKey();
	}

	AGeoProjectile* Projectile = GeoASLib::StartSpawnProjectile(GetWorld(), ProjectileClass, SpawnTransform,
																StoredPayload, GetEffectDataArray(), PredictionKey);
	if (!IsValid(Projectile))
	{
		ensureMsgf(false, TEXT("GeoDeployAbility: Failed to spawn projectile!"));
		return;
	}

	Projectile->OverrideDistanceSpan(StoredPayload.Seed);
	ADeployableSpawnerProjectile* DeployableSpawnerProjectile = Cast<ADeployableSpawnerProjectile>(Projectile);
	checkf(DeployableSpawnerProjectile, TEXT("SpawnerProjectile  must be a ADeployableSpawnerProjectile"));
	DeployableSpawnerProjectile->Params = Params;
	DeployableSpawnerProjectile->DeployableActorClass = DeployableActorClass;
	if (bOverrideDistanceSpan)
	{
		Projectile->OverrideDistanceSpan(DistanceSpan);
	}
	if (bOverrideSpeed)
	{
		Projectile->OverrideSpeed(ProjectileSpeed);
	}

	GeoASLib::FinishSpawnProjectile(GetWorld(), Projectile, SpawnTransform, SpawnServerTime, PredictionKey);
}
