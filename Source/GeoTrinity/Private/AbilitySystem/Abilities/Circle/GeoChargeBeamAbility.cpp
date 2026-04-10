// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Circle/GeoChargeBeamAbility.h"

#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"

UGeoChargeBeamAbility::UGeoChargeBeamAbility()
{
	FireMode = EFireMode::ChargeForFireDelay;
}

// ---------------------------------------------------------------------------------------------------------------------
FGeoAbilityTargetData UGeoChargeBeamAbility::BuildAbilityTargetData()
{
	// Encode charge ratio as integer 0–100 in Seed for server replication
	StoredPayload.Seed = FMath::RoundToInt(GetChargeRatio() * 100.f);
	FGeoAbilityTargetData Data = Super::BuildAbilityTargetData();
	Data.Seed = StoredPayload.Seed; // Ensure to set it properly even if Super changes code.
	return Data;
}

// ---------------------------------------------------------------------------------------------------------------------
TArray<TInstancedStruct<FEffectData>> UGeoChargeBeamAbility::GetEffectDataArray() const
{
	TArray<TInstancedStruct<FEffectData>> Effects = Super::GetEffectDataArray();

	float const ChargeRatio = FMath::Clamp(static_cast<float>(StoredPayload.Seed) / 100.f, 0.f, 1.f);
	bool const bSweetSpot = ChargeRatio >= SweetSpotMinRatio && ChargeRatio <= SweetSpotMaxRatio;
	float const MultiplierValue =
		bSweetSpot ? MaxDamageMultiplier : FMath::Lerp(MinDamageMultiplier, MaxDamageMultiplier, ChargeRatio);

	TInstancedStruct<FContextDamageMultiplierEffectData> ChargeMultiplier;
	ChargeMultiplier.InitializeAs<FContextDamageMultiplierEffectData>();
	ChargeMultiplier.GetMutable<FContextDamageMultiplierEffectData>().Multiplier = FScalableFloat(MultiplierValue);
	Effects.Add(MoveTemp(ChargeMultiplier));

	return Effects;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoChargeBeamAbility::OnFireTargetDataReceived(FGameplayAbilityTargetDataHandle const& DataHandle,
													 FGameplayTag ApplicationTag)
{
	FGeoAbilityTargetData const* TargetData = static_cast<FGeoAbilityTargetData const*>(DataHandle.Get(0));
	ensureMsgf(TargetData, TEXT("GeoChargeBeamAbility: No FGeoAbilityTargetData found in DataHandle"));
	if (!TargetData)
	{
		return;
	}

	// Decode charge ratio from Seed before parent spawns the projectile and calls GetEffectDataArray()
	StoredPayload.Seed = TargetData->Seed;
	Super::OnFireTargetDataReceived(DataHandle, ApplicationTag);
}
