// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Deployable/HealingZone/GeoHealingZone.h"

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Tool/UGeoGameplayLibrary.h"

void AGeoHealingZone::Tick(float const DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (GeoLib::IsServer(GetWorld()) && IsActive() && !IsBlinking())
	{
		UGeoAbilitySystemComponent* const SourceASC = GeoASLib::GetGeoAscFromActor(Data.Owner);
		if (ensureMsgf(SourceASC, TEXT("AGeoHealingZone: missing ASC.")))
		{
			for (AActor* const Ally : GeoASLib::GetInteractableActors(
					 this, GeoASLib::GetTeamId(this), Data.Params.Attitude, /*bMustBeDamageable*/ true,
					 FVector2D(GetActorLocation()), Data.Params.Size, ETargetOverlapMode::IncludeRadius))
			{
				HealAlly(Ally, SourceASC);
			}
		}
	}
}

void AGeoHealingZone::HealAlly(AActor* Ally, UGeoAbilitySystemComponent* SourceASC)
{
	UGeoAbilitySystemComponent* TargetASC = GeoASLib::GetGeoAscFromActor(Ally);
	if (!TargetASC
		|| TargetASC->GetNumericAttribute(UGeoAttributeSetBase::GetHealthAttribute())
			>= TargetASC->GetNumericAttribute(UGeoAttributeSetBase::GetMaxHealthAttribute()))
	{
		return; // Neither heal nor pay for an ally already at full life.
	}

	FHealEffectData HealEffectData;
	HealEffectData.Amount = DrainMagnitudePerSecond;
	HealEffectData.bIsPerSecond = true;
	GeoASLib::ApplySingleEffectData(HealEffectData, SourceASC, TargetASC, Data.Level, Data.Seed, Data.AbilityTag);

	// The zone pays for what it healed — once per healed ally, so it burns down faster the more it reaches. Distinct
	// from the base class's flat per-second drain.
	FDamageEffectData HealingCostData;
	HealingCostData.Amount = DrainMagnitudePerSecond;
	HealingCostData.bIsPerSecond = true;
	HealingCostData.bSuppressGameplayCue = true;
	HealingCostData.bSuppressCombatStats = true;
	HealingCostData.bDoNotRedirectSacrifice = true;
	GeoASLib::ApplySingleEffectData(HealingCostData, SourceASC, GetAbilitySystemComponent(), Data.Level, Data.Seed,
									Data.AbilityTag);
}
