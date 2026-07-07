// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Square/GeoSacrificeDetonateAbility.h"

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "Settings/GameDataSettings.h"
#include "Tool/Team.h"
#include "Tool/UGeoGameplayLibrary.h"

UGeoSacrificeDetonateAbility::UGeoSacrificeDetonateAbility()
{
	// Shares its input with the channel ability: a held button must never chain-activate across the pair.
	bActivateOnFreshPressOnly = true;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSacrificeDetonateAbility::Fire(FGeoAbilityTargetData const& AbilityTargetData)
{
	Super::Fire(AbilityTargetData);

	if (GeoLib::IsDedicatedServer(this))
	{
		return; // Stop here on dedicated server it will all execute from the OnFireTargetDataReceived.
	}

	Detonate(AbilityTargetData);
	EndAbility(false);
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
	float const MaxRange = GetDefault<UGameDataSettings>()->GeneralSpellDistance;
	FVector2D const ForwardVector = FVector2D(FRotator(0, AbilityTargetData.Yaw, 0).Vector());
	UGeoAbilitySystemComponent* SourceASC = GetGeoAbilitySystemComponentFromActorInfo();
	float const SacrificeValue = SourceASC->GetNumericAttribute(UCharacterAttributeSet::GetSacrificeValueAttribute());

	if (GeoLib::IsServer(GetWorld()))
	{
		FDamageEffectData DamageEffect;
		DamageEffect.DamageAmount = FScalableFloat(BaseDamage.GetValueAtLevel(GetAbilityLevel()) + SacrificeValue);
		for (AActor* Target : GeoASLib::GetInteractableActorsInLine(
				 this, GeoASLib::GetTeamId(StoredPayload.Instigator), TeamAttitudeMask::Hostile, true,
				 AbilityTargetData.Origin, ForwardVector, MaxRange, LineHalfWidth))
		{
			GeoASLib::ApplySingleEffectData(DamageEffect, SourceASC, GeoASLib::GetGeoAscFromActor(Target),
											GetAbilityLevel(), AbilityTargetData.Seed, GetAbilityTag());
		}

		// Consume the armed sacrifice: the channel becomes activatable again and the ability bar swaps back.
		SourceASC->SetNumericAttributeBase(UCharacterAttributeSet::GetSacrificeValueAttribute(), 0.f);
		SourceASC->RemoveActiveEffectsWithGrantedTags(
			FGameplayTagContainer(FGeoGameplayTags::Get().Status_Square_DetonateReady));
	}

	if (IsLocallyControlled() && FireGameplayCueTag.IsValid())
	{
		FVector2D const Endpoint = AbilityTargetData.Origin + ForwardVector * MaxRange;

		FGameplayCueParameters CueParams;
		CueParams.Location = FVector(Endpoint, ArbitraryCharacterZ);
		CueParams.Normal = FRotator(0, AbilityTargetData.Yaw, 0).Vector();
		CueParams.Instigator = StoredPayload.Instigator;
		CueParams.AbilityLevel = GetAbilityLevel();
		CueParams.RawMagnitude = SacrificeValue; // Replicated attribute — read before the server-side reset.

		// Local-only: the call is already gated to the locally-controlled machine, so each client fires the cue itself.
		// ExecuteGameplayCue would additionally multicast on the host, double-playing it on clients.
		SourceASC->InvokeGameplayCueEvent(FireGameplayCueTag, EGameplayCueEvent::Executed, CueParams);
	}
}
