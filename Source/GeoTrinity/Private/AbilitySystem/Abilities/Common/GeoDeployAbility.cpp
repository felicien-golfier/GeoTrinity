// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Common/GeoDeployAbility.h"

#include "AbilitySystem/Abilities/Base/AbilityPayload.h"
#include "AbilitySystem/AttributeSet/GeoGemAttributeSet.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Actor/Projectile/DeployableSpawner/DeployableSpawnerProjectile.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "Actor/Projectile/GeoProjectileFXComponent.h"
#include "Characters/Component/GeoDeploySatelliteComponent.h"
#include "Characters/PlayableCharacter.h"
#include "GameplayCueManager.h"
#include "Gem/GeoGemCatalog.h"
#include "Settings/GameDataSettings.h"
#include "Tool/UGeoGameplayLibrary.h"

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
	UAbilitySystemComponent const* ASC = GetAbilitySystemComponentFromActorInfo();
	bool const bHasSurplus = ASC && ASC->HasMatchingGameplayTag(FGeoGameplayTags::Get().Gem_Core_Surplus);
	return MaxCharges + (bHasSurplus ? FMath::RoundToInt(GeoASLib::GetGemMagnitude("Surplus")) : 0);
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

	int32 const StackLimit = CooldownGE->GetStackLimitCount();
	ensureMsgf(StackLimit <= 0 || StackLimit >= GetMaxStacks(),
			   TEXT("%hs: %s's Cooldown GE caps its stacks at %d, below its %d charges; set its StackLimitCount to 0"),
			   __FUNCTION__, *GetName(), StackLimit, GetMaxStacks());
	// A pool a removed Surplus shrank may still hold more spent charges than it has.
	return FMath::Max(GetMaxStacks() - ASC->GetGameplayEffectCount(CooldownGE->GetClass(), nullptr), 0);
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
	// Encode deploy distance as integer cm in Seed so the server receives it
	StoredPayload.Seed = FMath::RoundToInt(GetChargedDeployDistance());
	return Super::GetUpdatedTargetData();
}

// ---------------------------------------------------------------------------------------------------------------------
float UGeoDeployAbility::GetChargedDeployDistance() const
{
	UGameDataSettings const* GameDataSettings = GetDefault<UGameDataSettings>();
	return FMath::Lerp(GameDataSettings->MinDeployDistance, GameDataSettings->MaxDeployDistance, GetChargeRatio());
}

// ---------------------------------------------------------------------------------------------------------------------
FDeployableDataParams UGeoDeployAbility::ApplyGemsToParams(UAbilitySystemComponent const& ASC,
														   FDeployableDataParams const& BaseParams)
{
	FDeployableDataParams GemParams = BaseParams;
	if (ensureMsgf(ASC.HasAttributeSetForAttribute(UGeoGemAttributeSet::GetDeployableHealthMultiplierAttribute()),
				   TEXT("%hs: %s deploys from an ASC without gem attributes"), __FUNCTION__, *ASC.GetName()))
	{
		GemParams.BlinkDuration *=
			ASC.GetNumericAttribute(UGeoGemAttributeSet::GetDeployableBlinkMultiplierAttribute());
		GemParams.LifeDrainMaxDuration /=
			ASC.GetNumericAttribute(UGeoGemAttributeSet::GetDeployableDrainMultiplierAttribute());
		GemParams.HealthMultiplier =
			ASC.GetNumericAttribute(UGeoGemAttributeSet::GetDeployableHealthMultiplierAttribute());
	}

	return GemParams;
}

// ---------------------------------------------------------------------------------------------------------------------
FDeployableDataParams UGeoDeployAbility::GetGemParams(float const DeployDistance) const
{
	UAbilitySystemComponent const* ASC = GetAbilitySystemComponentFromActorInfo();
	FDeployableDataParams GemParams = ApplyGemsToParams(*ASC, Params);
	if (ASC->HasMatchingGameplayTag(FGeoGameplayTags::Get().Gem_Core_Leverage))
	{
		UGameDataSettings const* GameDataSettings = GetDefault<UGameDataSettings>();
		float const DistanceRatio = FMath::GetRangePct(GameDataSettings->MinDeployDistance,
													   GameDataSettings->MaxDeployDistance, DeployDistance);
		GemParams.HealthMultiplier *=
			GameDataSettings->LeverageHealthMultiplier.Interpolate(FMath::Clamp(DistanceRatio, 0.f, 1.f));
	}
	return GemParams;
}

// ---------------------------------------------------------------------------------------------------------------------
AGeoDeployableBase* UGeoDeployAbility::SpawnDeployableAt(UAbilitySystemComponent& DeployerASC, FVector const& Location,
														 float const HealthFraction)
{
	UGeoDeployAbility const* DeployAbility = GeoASLib::GetGrantedAbility<UGeoDeployAbility>(DeployerASC);
	FGameplayAbilitySpec const* Spec =
		DeployAbility ? DeployerASC.FindAbilitySpecFromClass(DeployAbility->GetClass()) : nullptr;

	AGeoDeployableBase* Deployable = nullptr;
	if (ensureMsgf(Spec, TEXT("%hs: %s holds no deploy ability"), __FUNCTION__,
				   *DeployerASC.GetOwnerActor()->GetName()))
	{
		FAbilityPayload Payload;
		Payload.SourceOwner = DeployerASC.GetOwnerActor();
		Payload.SourceAvatar = DeployerASC.GetAvatarActor();
		Payload.AbilityLevel = Spec->Level;
		Payload.AbilityTag = GeoASLib::GetAbilityTagFromSpec(*Spec);
		Payload.Seed = FMath::Rand32();

		FDeployableDataParams SpawnParams = ApplyGemsToParams(DeployerASC, DeployAbility->Params);
		SpawnParams.HealthMultiplier *= HealthFraction;
		Deployable =
			GeoASLib::FullySpawnDeployable(DeployAbility->DeployableActorClass, Payload,
										   DeployAbility->GetEffectDataArray(), SpawnParams, FTransform(Location));
	}

	return Deployable;
}

// ---------------------------------------------------------------------------------------------------------------------
FVector UGeoDeployAbility::GetPendingDeployLocation() const
{
	// Mirrors SpawnProjectile: an explicit DistanceSpan override replaces the charge-derived distance.
	float const DeployDistance = ProjectileParams.OverrideDistanceSpan == EOverrideParam::OverrideValue
		? ProjectileParams.DistanceSpan
		: GetChargedDeployDistance();
	UGeoAbilitySystemComponent* ASC = GetGeoAbilitySystemComponentFromActorInfo();
	FVector const Origin = GetFireOrigin(StoredPayload.SourceAvatar, ASC, StoredPayload.Seed);
	float const Yaw = GetFireYaw(StoredPayload.SourceAvatar, StoredPayload.Seed);
	return Origin
		+ FRotator(0.f, Yaw, 0.f).Vector() * DeployDistance
		* GeoASLib::GetStatValue(ASC, UGeoGemAttributeSet::GetSpellDistanceMultiplierAttribute(), 1.f);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoDeployAbility::SetChargeGaugeVisible(APlayableCharacter* Character, bool const bVisible)
{
	Super::SetChargeGaugeVisible(Character, bVisible);

	FGeoCueParam const& TargetCue = GetDefault<UGameDataSettings>()->DeployTargetCue;
	if (TargetCue.IsValid() && GeoLib::IsLocalPlayerAvatar(Character))
	{
		FGameplayCueParameters CueParams = TargetCue.MakeCueParams(StoredPayload, GetPendingDeployLocation());
		CueParams.SourceObject = this;
		if (bVisible)
		{
			UGameplayCueManager::AddGameplayCue_NonReplicated(Character, TargetCue.CueTag, CueParams);
		}
		// EndAbility hides the gauge even when the activation bailed before charging: removing a cue never added would
		// spawn its actor only to remove it.
		else if (GetAbilitySystemComponentFromActorInfo()->GetTagCount(TargetCue.CueTag) > 0)
		{
			UGameplayCueManager::RemoveGameplayCue_NonReplicated(Character, TargetCue.CueTag, CueParams);
		}
	}
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
	DeployableSpawnerProjectile->Params = GetGemParams(SpawnParams.DistanceSpan);
	DeployableSpawnerProjectile->DeployableActorClass = DeployableActorClass;

	GeoASLib::FinishSpawnProjectile(GetWorld(), Projectile, SpawnTransform, SpawnServerTime, PredictionKey);

	FGeoCueParam const& TargetCue = GameDataSettings->DeployTargetCue;
	if (TargetCue.IsValid() && GeoLib::IsLocalPlayerAvatar(StoredPayload.SourceAvatar))
	{
		FVector const LandingLocation = SpawnTransform.GetLocation()
			+ SpawnTransform.GetRotation().Vector() * Projectile->ResolvedParams.DistanceSpan;
		UGameplayCueManager::AddGameplayCue_NonReplicated(Projectile, TargetCue.CueTag,
														  TargetCue.MakeCueParams(StoredPayload, LandingLocation));
	}

	// Cosmetic hand-off, and local by construction: only the machine rendering the ring holds satellites, so nowhere
	// else moves anything. The projectile itself stays on the fire socket, so the deployable lands where it always did
	// — a host and a remote client deploy identically.
	UGeoDeploySatelliteComponent* SatelliteRing = IsValid(StoredPayload.SourceAvatar)
		? StoredPayload.SourceAvatar->GetComponentByClass<UGeoDeploySatelliteComponent>()
		: nullptr;
	FVector LaunchLocation;
	if (SatelliteRing && SatelliteRing->LaunchSatellite(LaunchLocation))
	{
		Projectile->FXComponent->SetVisualLaunchLocation(LaunchLocation);
	}
}
