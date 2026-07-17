// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Circle/GeoChargeBeamAbility.h"

#include "AbilitySystem/Abilities/Circle/GeoSweetSpotChargePassiveAbility.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Characters/PlayableCharacter.h"
#include "Settings/GameDataSettings.h"
#include "Tool/Team.h"
#include "Tool/UGeoGameplayLibrary.h"

// Permillage headroom absorbing RPC jitter between the client's release and the server observing it.
static constexpr int32 ChargeRatioTolerancePermille = 100;

UGeoChargeBeamAbility::UGeoChargeBeamAbility()
{
	FireMode = EFireMode::ChargeForFireDelay;
	CommitBehaviour = ECommitBehaviour::AtActivate;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoChargeBeamAbility::SetChargeGaugeVisible(APlayableCharacter* Character, bool bVisible)
{
	if (!GeoLib::IsDedicatedServer(this))
	{
		Character->SetChargeBeamGaugeVisible(this, bVisible, SweetSpotMinRatio, SweetSpotMaxRatio);
	}
}

// ---------------------------------------------------------------------------------------------------------------------

FGeoAbilityTargetData UGeoChargeBeamAbility::GetUpdatedTargetData()
{
	// Seed field is repurposed to carry the charge ratio as an integer permillage (0–1000).
	// This piggybacks on the existing RPC without adding a new field, since Seed is unused by the beam otherwise.
	float const ChargeRatio = GetChargeRatio();
	StoredPayload.Seed = FMath::RoundToInt(ChargeRatio * 1000.f);
	return Super::GetUpdatedTargetData();
}

// ---------------------------------------------------------------------------------------------------------------------
float UGeoChargeBeamAbility::GetStoredChargeRatio() const
{
	return FMath::Clamp(static_cast<float>(StoredPayload.Seed) / 1000.f, 0.f, 1.f);
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoChargeBeamAbility::IsSweetSpotRelease() const
{
	float const ChargeRatio = GetStoredChargeRatio();
	return ChargeRatio >= SweetSpotMinRatio && ChargeRatio <= SweetSpotMaxRatio;
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
				ASC ? UGeoSweetSpotChargePassiveAbility::FindOnASC(*ASC) : nullptr;
			if (Passive && Passive->GetGaugeRatio(*ASC) >= 1.f)
			{
				// Full gauge: the release's damage comes solely from the healing the passive recorded.
				MultiplierValue = 1.f;
				FScalableFloat const BoostDamage(Passive->GetBoostDamage(*ASC));
				for (TInstancedStruct<FEffectData>& Effect : Effects)
				{
					if (FDamageEffectData* DamageEffect = Effect.GetMutablePtr<FDamageEffectData>())
					{
						DamageEffect->DamageAmount = BoostDamage;
					}
				}
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
	if (FireGameplayCueTag.IsValid())
	{
		FVector2D ForwardVector = FVector2D(FRotator(0, StoredPayload.Yaw, 0).Vector());
		ForwardVector *= GetDefault<UGameDataSettings>()->GeneralSpellDistance;

		float const ChargeRatio = GetStoredChargeRatio();

		FGameplayCueParameters CueParams;
		CueParams.Location = FVector(AbilityTargetData.Origin + ForwardVector, ArbitraryCharacterZ);
		CueParams.Instigator = StoredPayload.Instigator;
		CueParams.AbilityLevel = StoredPayload.AbilityLevel;
		CueParams.NormalizedMagnitude = IsSweetSpotRelease() || ChargeRatio >= .95f;
		CueParams.Normal = FRotator(0, AbilityTargetData.Yaw, 0).Vector();
		CueParams.RawMagnitude = ChargeRatio;
		UAbilitySystemComponent* const ASC = GetAbilitySystemComponentFromActorInfo();
		FScopedPredictionWindow ScopedPredictionWindow(ASC);
		ASC->ExecuteGameplayCue(FireGameplayCueTag, CueParams);
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
			DealDamage(AbilityTargetData);
		}
		FireGameplayCue(AbilityTargetData);
		EndAbility(false);
	}
}

void UGeoChargeBeamAbility::DealDamage(FGeoAbilityTargetData const& AbilityTargetData) const
{
	UGeoAbilitySystemComponent* SourceASC = GetGeoAbilitySystemComponentFromActorInfo();
	if (!ensureMsgf(SourceASC, TEXT("UGeoChargeBeamAbility: invalid ASC on server")))
	{
		return;
	}

	float const MaxRange = GetDefault<UGameDataSettings>()->GeneralSpellDistance;
	FVector2D const ForwardVector = FVector2D(FRotator(0, AbilityTargetData.Yaw, 0).Vector());

	AActor const* const Avatar = GetAvatarActorFromActorInfo();
	for (AActor* Target :
		 GeoASLib::GetInteractableActorsInLine(this, GeoASLib::GetTeamId(Avatar), TeamAttitudeMask::HostileOrNeutral,
											   true, AbilityTargetData.Origin, ForwardVector, MaxRange))
	{
		if (Target == Avatar)
		{
			continue;
		}

		UGeoAbilitySystemComponent* TargetASC = GeoASLib::GetGeoAscFromActor(Target);
		if (!IsValid(TargetASC))
		{
			continue;
		}

		GeoASLib::ApplyEffectFromEffectData(GetEffectDataArray(), SourceASC, TargetASC, GetAbilityLevel(),
											AbilityTargetData.Seed, GetAbilityTag());
	}

	// A sweet-spot release spends the full gauge whether or not it hit anything — the boosted beam was fired.
	UGeoSweetSpotChargePassiveAbility const* Passive = UGeoSweetSpotChargePassiveAbility::FindOnASC(*SourceASC);
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

	StoredPayload.Seed =
		FMath::Min(StoredPayload.Seed, FMath::RoundToInt(GetChargeRatio() * 1000.f) + ChargeRatioTolerancePermille);
	ClampRemoteClientOrigin();

	FGeoAbilityTargetData const* AbilityTargetData = static_cast<FGeoAbilityTargetData const*>(DataHandle.Get(0));
	if (!ensureMsgf(AbilityTargetData,
					TEXT("No FGeoAbilityTargetData found in DataHandle — cannot update StoredPayload.")))
	{
		EndAbility(true, true);
		return;
	}

	DealDamage(*AbilityTargetData);
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}
