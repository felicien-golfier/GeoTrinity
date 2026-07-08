// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Circle/GeoChargeBeamAbility.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Characters/PlayableCharacter.h"
#include "Settings/GameDataSettings.h"
#include "Tool/Team.h"
#include "Tool/UGeoGameplayLibrary.h"

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
TArray<TInstancedStruct<FEffectData>> UGeoChargeBeamAbility::GetEffectDataArray() const
{
	TArray<TInstancedStruct<FEffectData>> Effects = Super::GetEffectDataArray();

	float const ChargeRatio = FMath::Clamp(static_cast<float>(StoredPayload.Seed) / 1000.f, 0.f, 1.f);
	bool const bSweetSpot = ChargeRatio >= SweetSpotMinRatio && ChargeRatio <= SweetSpotMaxRatio;
	float const MultiplierValue =
		bSweetSpot ? SweetSpotDamageMultiplier : FMath::Lerp(MinDamageMultiplier, MaxDamageMultiplier, ChargeRatio);

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

		float const ChargeRatio = FMath::Clamp(static_cast<float>(StoredPayload.Seed) / 1000.f, 0.f, 1.f);

		FGameplayCueParameters CueParams;
		CueParams.Location = FVector(AbilityTargetData.Origin + ForwardVector, ArbitraryCharacterZ);
		CueParams.Instigator = StoredPayload.Instigator;
		CueParams.AbilityLevel = StoredPayload.AbilityLevel;
		CueParams.NormalizedMagnitude =
			ChargeRatio >= SweetSpotMinRatio && ChargeRatio <= SweetSpotMaxRatio || ChargeRatio >= .95f;
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
}
// ---------------------------------------------------------------------------------------------------------------------
void UGeoChargeBeamAbility::OnFireTargetDataReceived(FGameplayAbilityTargetDataHandle const& DataHandle,
													 FGameplayTag const ApplicationTag)
{
	// Call Super first to get the Payload Seed updated.
	Super::OnFireTargetDataReceived(DataHandle, ApplicationTag);

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
