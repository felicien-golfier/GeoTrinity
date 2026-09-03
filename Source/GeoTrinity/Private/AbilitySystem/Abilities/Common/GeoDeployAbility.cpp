// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Common/GeoDeployAbility.h"

#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystemComponent.h"
#include "Actor/Projectile/DeployableSpawner/DeployableSpawnerProjectile.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "Characters/Component/GeoDeploySatelliteComponent.h"
#include "Settings/GameDataSettings.h"

UGeoDeployAbility::UGeoDeployAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	FireMode = EFireMode::ChargeForFireDelay;
	CommitBehaviour = ECommitBehaviour::DoNotAutoCommit;
	ProjectileParams.OverrideSpeed = EOverrideParam::OverrideValue;
	ProjectileParams.ProjectileSpeed = 2000.f;
	bActivateOnFreshPressOnly = true;
}

// ---------------------------------------------------------------------------------------------------------------------
int32 UGeoDeployAbility::GetMaxStacks() const
{
	UGameplayEffect const* CooldownGE = GetCooldownGameplayEffect();
	if (!ensureMsgf(CooldownGE, TEXT("GeoDeployAbility '%s': no cooldown GE; the charge pool is its StackLimitCount."),
					*GetName()))
	{
		return 0;
	}
	return CooldownGE->GetStackLimitCount();
}

// ---------------------------------------------------------------------------------------------------------------------
int32 UGeoDeployAbility::GetCurrentStacks() const
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	UGameplayEffect const* CooldownGE = GetCooldownGameplayEffect();
	if (!ASC || !CooldownGE)
	{
		return 0;
	}
	return GetMaxStacks() - ASC->GetGameplayEffectCount(CooldownGE->GetClass(), nullptr);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoDeployAbility::OnGiveAbility(FGameplayAbilityActorInfo const* ActorInfo, FGameplayAbilitySpec const& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	LastKnownStacks = GetMaxStacks();

	FGameplayTagContainer const* CooldownTags = GetCooldownTags();
	ensureMsgf(CooldownTags && !CooldownTags->IsEmpty(),
			   TEXT("GeoDeployAbility '%s': cooldown GE grants no tags; the refill sound cannot be tracked."),
			   *GetName());
	if (CooldownTags && !CooldownTags->IsEmpty())
	{
		CooldownTagDelegateHandle =
			ActorInfo->AbilitySystemComponent
				->RegisterGameplayTagEvent(CooldownTags->First(), EGameplayTagEventType::AnyCountChange)
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
																	  EGameplayTagEventType::AnyCountChange);
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

	// Spend a charge only once the activation has actually taken — Super calls EndAbility (clearing IsActive) if it
	// bails on cost.
	if (!IsActive())
	{
		return;
	}

	CommitAbilityCooldown(Handle, ActorInfo, ActivationInfo, true);
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoDeployAbility::CanActivateAbility(FGameplayAbilitySpecHandle const Handle,
										   FGameplayAbilityActorInfo const* ActorInfo,
										   FGameplayTagContainer const* SourceTags,
										   FGameplayTagContainer const* TargetTags,
										   FGameplayTagContainer* OptionalRelevantTags) const
{
	return GetCurrentStacks() > 0
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
void UGeoDeployAbility::OnCooldownTagChanged(FGameplayTag const /*CooldownTag*/, int32 const /*NewCount*/)
{
	// NewCount tracks the tag's presence, not the pool: AnyCountChange also fires from Notify_StackCountChange, where
	// the count is unchanged. Re-read the pool to tell a refill from a spend.
	int32 const NewStacks = GetCurrentStacks();
	bool const bRefilled = NewStacks > LastKnownStacks;
	LastKnownStacks = NewStacks;

	AActor* const Avatar = GetAvatarActorFromActorInfo();
	if (bRefilled && IsLocallyControlled() && IsValid(Avatar))
	{
		FGeoCueParam const& RefillCue = GetDefault<UGameDataSettings>()->RefillDeployableCue;

		// Each client fires the cue itself off its own predicted/replicated tag edge.
		GeoASLib::ExecuteGeoCue(
			GetAbilitySystemComponentFromActorInfo(), RefillCue,
			RefillCue.MakeCueParams(Avatar, Avatar, Avatar->GetActorLocation(), GetAbilityLevel(), GetAbilityTag()),
			true);
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
	checkf(ProjectileParams.ProjectileClass, TEXT("No ProjectileClass set on GeoDeployAbility!"));

	FPredictionKey PredictionKey;
	EGameplayAbilityActivationMode::Type const ActivationMode = GetCurrentActivationInfo().ActivationMode;
	if (ActivationMode == EGameplayAbilityActivationMode::Predicting
		|| ActivationMode == EGameplayAbilityActivationMode::Confirmed
		|| ActivationMode == EGameplayAbilityActivationMode::Authority)
	{
		PredictionKey = GetCurrentActivationInfo().GetActivationPredictionKey();
	}

	// The charge-derived deploy distance is the default; an explicit ProjectileParams override (if set) takes over.
	UGameDataSettings const* GameDataSettings = GetDefault<UGameDataSettings>();
	FExternalProjectileParams SpawnParams = ProjectileParams;
	if (SpawnParams.OverrideDistanceSpan != EOverrideParam::OverrideValue)
	{
		SpawnParams.OverrideDistanceSpan = EOverrideParam::OverrideValue;
		SpawnParams.DistanceSpan =
			FMath::Clamp(StoredPayload.Seed, FMath::RoundToInt(GameDataSettings->MinDeployDistance),
						 FMath::RoundToInt(GameDataSettings->MaxDeployDistance));
	}

	AGeoProjectile* Projectile = GeoASLib::StartSpawnProjectile(GetWorld(), SpawnParams, SpawnTransform, StoredPayload,
																GetEffectDataArray(), PredictionKey);
	if (!ensureMsgf(IsValid(Projectile), TEXT("%hs: failed to spawn projectile"), __FUNCTION__))
	{
		return;
	}

	ADeployableSpawnerProjectile* DeployableSpawnerProjectile = Cast<ADeployableSpawnerProjectile>(Projectile);
	checkf(DeployableSpawnerProjectile, TEXT("SpawnerProjectile  must be a ADeployableSpawnerProjectile"));
	DeployableSpawnerProjectile->Params = Params;
	DeployableSpawnerProjectile->DeployableActorClass = DeployableActorClass;

	GeoASLib::FinishSpawnProjectile(GetWorld(), Projectile, SpawnTransform, SpawnServerTime, PredictionKey);

	// Cosmetic hand-off, and local by construction: only the machine rendering the ring holds satellites, so nowhere
	// else moves anything. The projectile itself stays on the fire socket, so the deployable lands where it always did
	// — a host and a remote client deploy identically.
	UGeoDeploySatelliteComponent* SatelliteRing = IsValid(StoredPayload.SourceAvatar)
		? StoredPayload.SourceAvatar->GetComponentByClass<UGeoDeploySatelliteComponent>()
		: nullptr;
	FVector LaunchLocation;
	if (SatelliteRing && SatelliteRing->LaunchSatellite(LaunchLocation))
	{
		Projectile->SetVisualLaunchLocation(LaunchLocation);
	}
}
