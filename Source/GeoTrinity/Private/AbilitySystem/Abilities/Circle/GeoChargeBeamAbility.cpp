// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Circle/GeoChargeBeamAbility.h"

#include "AbilitySystem/Abilities/Circle/GeoSweetSpotChargePassiveAbility.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "Characters/Component/GeoCharacterMovementComponent.h"
#include "Characters/PlayableCharacter.h"
#include "Settings/GameDataSettings.h"
#include "Tool/Team.h"
#include "Tool/UGeoGameplayLibrary.h"

// Hold a remote client may claim beyond the one the server saw between its activation and its release arriving.
static constexpr float ServerHoldTolerance = 0.1f;
// Beams a remote client may redeem at once after idling, so a hitch bunching two together costs no legitimate one.
static constexpr float MaxServerBeamBurst = 1.f;

UGeoChargeBeamAbility::UGeoChargeBeamAbility()
{
	FireMode = EFireMode::ChargeForFireDelay;
	CommitBehaviour = ECommitBehaviour::DoNotAutoCommit;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

// ---------------------------------------------------------------------------------------------------------------------
FGameplayTag UGeoChargeBeamAbility::GetAlternateReleaseInputTag() const
{
	return FGeoGameplayTags::Get().InputTag_Reload;
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoChargeBeamAbility::CheckCooldown(FGameplayAbilitySpecHandle /*Handle*/,
										  FGameplayAbilityActorInfo const* ActorInfo,
										  FGameplayTagContainer* /*OptionalRelevantTags*/) const
{
	return !ActorInfo->IsLocallyControlled()
		|| ActorInfo->AbilitySystemComponent->GetWorld()->GetTimeSeconds() >= NextAllowedShotTime;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoChargeBeamAbility::GetCooldownTimeRemainingAndDuration(FGameplayAbilitySpecHandle const Handle,
																FGameplayAbilityActorInfo const* ActorInfo,
																float& TimeRemaining, float& CooldownDuration) const
{
	float const Now = ActorInfo->AbilitySystemComponent->GetWorld()->GetTimeSeconds();
	TimeRemaining = FMath::Max(NextAllowedShotTime - Now, 0.f);
	CooldownDuration = GetCooldown(GetAbilityLevel(Handle, ActorInfo));
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoChargeBeamAbility::SetChargeGaugeVisible(APlayableCharacter* Character, bool bVisible)
{
	if (GeoLib::IsLocalPlayerAvatar(Character))
	{
		Character->SetChargeBeamGaugeVisible(this, bVisible, SweetSpotMinRatio, SweetSpotMaxRatio);
	}
}

// ---------------------------------------------------------------------------------------------------------------------

FGeoAbilityTargetData UGeoChargeBeamAbility::GetUpdatedTargetData()
{
	SetStoredHeldSeconds(GetChargeElapsedSeconds());
	return Super::GetUpdatedTargetData();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoChargeBeamAbility::SetStoredHeldSeconds(float const HeldSeconds)
{
	StoredPayload.Seed = FMath::RoundToInt(HeldSeconds * 1000.f);
}

// ---------------------------------------------------------------------------------------------------------------------
float UGeoChargeBeamAbility::GetStoredHeldSeconds() const
{
	return StoredPayload.Seed / 1000.f;
}

// ---------------------------------------------------------------------------------------------------------------------
float UGeoChargeBeamAbility::GetStoredChargeRatio() const
{
	return ApplyChargingCurve(FMath::Clamp(GetStoredHeldSeconds() / GetFireDelay(), 0.f, 1.f));
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoChargeBeamAbility::IsSweetSpotRelease() const
{
	float const ChargeRatio = GetStoredChargeRatio();
	return ChargeRatio >= SweetSpotMinRatio && ChargeRatio <= SweetSpotMaxRatio;
}

// ---------------------------------------------------------------------------------------------------------------------
float UGeoChargeBeamAbility::GetSweetSpotPrecision() const
{
	float const HalfWidth = (SweetSpotMaxRatio - SweetSpotMinRatio) * 0.5f;
	if (HalfWidth <= 0.f)
	{
		return 1.f;
	}
	float const Center = (SweetSpotMinRatio + SweetSpotMaxRatio) * 0.5f;
	return FMath::Clamp(1.f - FMath::Abs(GetStoredChargeRatio() - Center) / HalfWidth, 0.f, 1.f);
}

// ---------------------------------------------------------------------------------------------------------------------
TArray<TInstancedStruct<FEffectData>> UGeoChargeBeamAbility::GetEffectDataArray() const
{
	TArray<TInstancedStruct<FEffectData>> Effects = Super::GetEffectDataArray();

	float MultiplierValue = FMath::Lerp(MinDamageMultiplier, MaxDamageMultiplier, GetStoredChargeRatio());
	if (IsSweetSpotRelease())
	{
		MultiplierValue = SweetSpotDamageMultiplier;
		// GetCurrentActorInfo is null when the CDO resolves ability descriptions — no ASC, no gauge to read.
		if (GetCurrentActorInfo())
		{
			UAbilitySystemComponent const* ASC = GetAbilitySystemComponentFromActorInfo();
			UGeoSweetSpotChargePassiveAbility const* Passive =
				ASC ? GeoASLib::GetGrantedAbility<UGeoSweetSpotChargePassiveAbility>(*ASC) : nullptr;
			if (Passive && Passive->GetGaugeRatio(*ASC) >= 1.f)
			{
				MultiplierValue += Passive->GetHealsToDamageMultiplier(GetSweetSpotPrecision());
			}
		}
	}

	TInstancedStruct<FContextDamageMultiplierEffectData> ChargeMultiplier;
	ChargeMultiplier.InitializeAs<FContextDamageMultiplierEffectData>();
	ChargeMultiplier.GetMutable<FContextDamageMultiplierEffectData>().Multiplier = FScalableFloat(MultiplierValue);
	Effects.Add(MoveTemp(ChargeMultiplier));

	return Effects;
}

void UGeoChargeBeamAbility::FireGameplayCue(FGeoAbilityTargetData const& AbilityTargetData)
{
	if (FireCue.IsValid())
	{
		FVector2D ForwardVector = FVector2D(FRotator(0, StoredPayload.Yaw, 0).Vector());
		ForwardVector *= GetDefault<UGameDataSettings>()->GeneralSpellDistance;

		float const ChargeRatio = GetStoredChargeRatio();

		FGameplayCueParameters CueParams = FireCue.MakeCueParams(
			StoredPayload, FVector(AbilityTargetData.Origin + ForwardVector, ArbitraryCharacterZ));
		CueParams.NormalizedMagnitude = IsSweetSpotRelease() || ChargeRatio >= .95f;
		CueParams.Normal = FRotator(0, AbilityTargetData.Yaw, 0).Vector();
		CueParams.RawMagnitude = ChargeRatio;
		GeoASLib::ExecuteGeoCue(GetAbilitySystemComponentFromActorInfo(), FireCue, CueParams, false);
	}
}
// ---------------------------------------------------------------------------------------------------------------------
void UGeoChargeBeamAbility::Fire(FGeoAbilityTargetData const& AbilityTargetData)
{
	Super::Fire(AbilityTargetData);
	if (IsLocallyControlled())
	{
		if (GeoLib::IsServer(this)) // Host Case
		{
			DealDamage();
		}

		FireGameplayCue(AbilityTargetData);
		NextAllowedShotTime = GetWorld()->GetTimeSeconds() + GetCooldown(GetAbilityLevel());
		EndAbility(false);
	}
}

void UGeoChargeBeamAbility::DealDamage() const
{
	UGeoAbilitySystemComponent* SourceASC = GetGeoAbilitySystemComponentFromActorInfo();
	if (!ensureMsgf(SourceASC, TEXT("UGeoChargeBeamAbility: invalid ASC on server")))
	{
		return;
	}

	float const MaxRange = GetDefault<UGameDataSettings>()->GeneralSpellDistance;
	FVector2D const ForwardVector = FVector2D(FRotator(0, StoredPayload.Yaw, 0).Vector());

	AActor const* const Avatar = GetAvatarActorFromActorInfo();
	FGenericTeamId const SourceTeam = GeoASLib::GetTeamId(Avatar);
	float const SeenServerTime = GeoLib::GetPerceivedServerTime(Avatar) - GeoLib::GetReplicationDelay(Avatar);
	TArray<TInstancedStruct<FEffectData>> const Effects = GetEffectDataArray();
	for (AActor* Target : GeoASLib::GetInteractableActors(this, SourceTeam, TeamAttitudeMask::HostileOrNeutral,
														  /*bMustBeDamageable*/ true))
	{
		FVector2D const SeenLocation(GeoLib::GetPoseAt(Target, SeenServerTime).Location);
		if (Target == Avatar
			|| !GeoASLib::IsInLine(Target, SeenLocation, StoredPayload.Origin, ForwardVector, MaxRange,
								   /*LineHalfWidth*/ 0.f, ETargetOverlapMode::Automatic, SourceTeam))
		{
			continue;
		}

		UGeoAbilitySystemComponent* TargetASC = GeoASLib::GetGeoAscFromActor(Target);
		if (!IsValid(TargetASC))
		{
			continue;
		}

		GeoASLib::ApplyEffectFromEffectData(Effects, SourceASC, TargetASC, GetAbilityLevel(), StoredPayload.Seed,
											GetAbilityTag());
		GeoASLib::NotifyAbilityHit(StoredPayload, Target);
	}

	// A sweet-spot release spends the full gauge whether or not it hit anything — the boosted beam was fired.
	UGeoSweetSpotChargePassiveAbility const* Passive =
		GeoASLib::GetGrantedAbility<UGeoSweetSpotChargePassiveAbility>(*SourceASC);
	if (IsSweetSpotRelease() && Passive && Passive->GetGaugeRatio(*SourceASC) >= 1.f)
	{
		Passive->ConsumeGauge(*SourceASC);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoChargeBeamAbility::OnFireTargetDataReceived(FGameplayAbilityTargetDataHandle const& DataHandle,
													 FGameplayTag const ApplicationTag)
{
	// Call Super first to get the Payload Seed updated.
	Super::OnFireTargetDataReceived(DataHandle, ApplicationTag);
	ClampRemoteClientOrigin();

	float const ServerSeenHold = GetChargeElapsedSeconds();
	SetStoredHeldSeconds(FMath::Clamp(GetStoredHeldSeconds(), 0.f, ServerSeenHold + ServerHoldTolerance));

	bool const bOnSchedule = TryConsumeShotSlot(
		ChargeStartTime, GetStoredHeldSeconds() + GetCooldown(GetAbilityLevel()), MaxServerBeamBurst);
	if (bOnSchedule)
	{
		DealDamage();
	}

	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, !bOnSchedule);
}
