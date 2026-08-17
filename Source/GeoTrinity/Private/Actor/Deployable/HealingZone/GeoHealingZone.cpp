// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Deployable/HealingZone/GeoHealingZone.h"

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"

void AGeoHealingZone::ApplyZoneEffects(TWeakObjectPtr<AActor> const& TrackedActor,
									   UGeoAbilitySystemComponent* SourceASC, float const DeltaSeconds)
{
	AActor* Actor = TrackedActor.Get();
	UGeoAbilitySystemComponent* TargetASC = GeoASLib::GetGeoAscFromActor(Actor);
	if (!TargetASC || !TargetASC->GetAvatarActor()->CanBeDamaged()
		|| TargetASC->GetNumericAttribute(UGeoAttributeSetBase::GetHealthAttribute())
			>= TargetASC->GetNumericAttribute(UGeoAttributeSetBase::GetMaxHealthAttribute()))
	{
		return; // Neither heal nor pay for an ally already at full life.
	}

	// The authored array on top, so game design can add something to the heal without touching this class.
	Super::ApplyZoneEffects(TrackedActor, SourceASC, DeltaSeconds);

	FHealEffectData HealEffectData;
	HealEffectData.HealAmount = DrainMagnitudePerSecond * DeltaSeconds;
	HealEffectData.bLimitGameplayCue = true;
	GeoASLib::ApplySingleEffectData(HealEffectData, SourceASC, TargetASC, Data.Level, Data.Seed, Data.AbilityTag);

	// The zone pays for what it healed — once per healed ally, so it burns down faster the more it reaches. Distinct
	// from the base class's flat per-second drain.
	FDamageEffectData HealingCostData;
	HealingCostData.DamageAmount = DrainMagnitudePerSecond * DeltaSeconds;
	HealingCostData.bSuppressGameplayCue = true;
	HealingCostData.bSuppressCombatStats = true;
	HealingCostData.bDoNotRedirectSacrifice = true;
	GeoASLib::ApplySingleEffectData(HealingCostData, SourceASC, GetAbilitySystemComponent(), Data.Level, Data.Seed,
									Data.AbilityTag);
}
