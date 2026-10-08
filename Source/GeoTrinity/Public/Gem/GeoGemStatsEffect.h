// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"

#include "GeoGemStatsEffect.generated.h"

class UGeoGemCatalog;
struct FGeoGemInfo;
struct FGeoGemLoadout;
enum class EGeoGemOperation : uint8;

/**
 * Infinite effect carrying every stat a loadout's gems give, as one active effect: one Add and one Percent modifier
 * per gem attribute, each fed a SetByCaller magnitude named after its attribute and operation, plus the tags its gems
 * grant. A loadout change removes it and applies a fresh spec. Native so it needs no asset; a gem on an attribute not
 * listed in the constructor is flagged when the spec is made.
 */
UCLASS()
class GEOTRINITY_API UGeoGemStatsEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	/** Lists the gem attributes, UGeoGemAttributeSet's included, with their Add and Percent modifiers. */
	UGeoGemStatsEffect();

	/** Spec giving ASC the summed stats of Loadout's gems, neutral on every attribute no slotted gem raises, and the tag
	 *  of every slotted gem that grants one. */
	static FGameplayEffectSpecHandle MakeSpec(UAbilitySystemComponent const& ASC, UGeoGemCatalog const& Catalog,
											  FGeoGemLoadout const& Loadout);

	/** True when Gem raises no stat, or one of the attributes this effect lists with Gem's operation. */
	static bool Supports(FGeoGemInfo const& Gem);

private:
	/** SetByCaller name of Attribute's modifier for Operation. */
	static FName GetSetByCallerName(FGameplayAttribute const& Attribute, EGeoGemOperation Operation);
};
