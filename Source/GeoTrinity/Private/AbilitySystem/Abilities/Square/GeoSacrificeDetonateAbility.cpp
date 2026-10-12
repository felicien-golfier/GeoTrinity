// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Square/GeoSacrificeDetonateAbility.h"

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "Tool/Team.h"
#include "Tool/UGeoGameplayLibrary.h"

UGeoSacrificeDetonateAbility::UGeoSacrificeDetonateAbility()
{
	// Shares its input with the channel ability: a held button must never chain-activate across the pair.
	bActivateOnFreshPressOnly = true;
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoSacrificeDetonateAbility::IsReady(UAbilitySystemComponent const& ASC) const
{
	return ASC.HasMatchingGameplayTag(FGeoGameplayTags::Get().Status_Square_DetonateReady);
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoSacrificeDetonateAbility::GetClassGauge(UAbilitySystemComponent const& ASC, float& OutFill,
												  bool& bOutReady) const
{
	float const Sacrificed = ASC.GetNumericAttribute(UCharacterAttributeSet::GetSacrificeValueAttribute());
	OutFill = FMath::Clamp(Sacrificed / SacrificeForFullGauge, 0.f, 1.f);
	bOutReady = IsReady(ASC);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoSacrificeDetonateAbility::GetGaugeLabel(UAbilitySystemComponent const& ASC, FText& OutLabel) const
{
	int32 const Sacrificed =
		FMath::RoundToInt(ASC.GetNumericAttribute(UCharacterAttributeSet::GetSacrificeValueAttribute()));
	OutLabel = Sacrificed > 0 ? FText::AsNumber(Sacrificed) : FText::GetEmpty();
	return Sacrificed > 0;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSacrificeDetonateAbility::Fire(FGeoAbilityTargetData const& AbilityTargetData)
{
	Super::Fire(AbilityTargetData);

	if (IsLocallyControlled())
	{
		Detonate(AbilityTargetData);
		EndAbility(false);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSacrificeDetonateAbility::OnFireTargetDataReceived(FGameplayAbilityTargetDataHandle const& DataHandle,
															FGameplayTag const ApplicationTag)
{
	Super::OnFireTargetDataReceived(DataHandle, ApplicationTag);

	FGeoAbilityTargetData const* AbilityTargetData = static_cast<FGeoAbilityTargetData const*>(DataHandle.Get(0));
	if (!ensureMsgf(AbilityTargetData,
					TEXT("UGeoSacrificeDetonateAbility: no FGeoAbilityTargetData in DataHandle — cannot detonate.")))
	{
		EndAbility(true, true);
		return;
	}

	Detonate(*AbilityTargetData);
	EndAbility(true, false);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSacrificeDetonateAbility::Detonate(FGeoAbilityTargetData const& AbilityTargetData)
{
	UGeoAbilitySystemComponent* SourceASC = GetGeoAbilitySystemComponentFromActorInfo();
	float const MaxRange = GeoASLib::GetSpellDistance(SourceASC);
	FVector2D const ForwardVector = FVector2D(FRotator(0, AbilityTargetData.Yaw, 0).Vector());
	float const SacrificeValue = SourceASC->GetNumericAttribute(UCharacterAttributeSet::GetSacrificeValueAttribute());

	if (GeoLib::IsServer(GetWorld()))
	{
		FDamageEffectData DamageEffect;
		DamageEffect.Amount =
			FScalableFloat(BaseDamage.GetValueAtLevel(GetAbilityLevel()) + SacrificeValue * SacrificeValueMultiplier);
		for (AActor* Target : GeoASLib::GetInteractableActorsInLine(
				 this, GeoASLib::GetTeamId(StoredPayload.SourceOwner), TeamAttitudeMask::Hostile, true,
				 AbilityTargetData.Origin, ForwardVector, MaxRange, LineHalfWidth))
		{
			GeoASLib::ApplySingleEffectData(DamageEffect, SourceASC, GeoASLib::GetGeoAscFromActor(Target),
											GetAbilityLevel(), AbilityTargetData.Seed, GetAbilityTag());
			GeoASLib::NotifyAbilityHit(StoredPayload, Target);
		}

		// Consume the armed sacrifice: the channel becomes activatable again and the ability bar swaps back.
		SourceASC->SetNumericAttributeBase(UCharacterAttributeSet::GetSacrificeValueAttribute(), 0.f);
		SourceASC->RemoveActiveEffectsWithGrantedTags(
			FGameplayTagContainer(FGeoGameplayTags::Get().Status_Square_DetonateReady));
	}

	if (IsLocallyControlled() && FireCue.IsValid())
	{
		FVector2D const Endpoint = AbilityTargetData.Origin + ForwardVector * MaxRange;

		FGameplayCueParameters CueParams = FireCue.MakeCueParams(StoredPayload, FVector(Endpoint, ArbitraryCharacterZ));
		CueParams.Normal = FRotator(0, AbilityTargetData.Yaw, 0).Vector();
		CueParams.RawMagnitude = SacrificeValue; // Replicated attribute — read before the server-side reset.

		GeoASLib::ExecuteGeoCue(SourceASC, FireCue, CueParams, true);
	}
}
