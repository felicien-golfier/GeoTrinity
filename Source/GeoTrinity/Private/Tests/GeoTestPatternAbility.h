// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/PatternAbility.h"
#include "AbilitySystem/Abilities/Pattern/Pattern.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "Animation/AnimMontage.h"
#include "CoreMinimal.h"

#include "GeoTestPatternAbility.generated.h"

/** Pattern of a test that plays a montage of its own. */
UCLASS(NotPlaceable, Transient)
class UGeoTestMontagePattern : public UPattern
{
	GENERATED_BODY()

public:
	UGeoTestMontagePattern() { AnimMontage = CreateDefaultSubobject<UAnimMontage>(TEXT("Montage")); }
};

/** Pattern of a test with no montage. */
UCLASS(NotPlaceable, Transient)
class UGeoTestSilentPattern : public UPattern
{
	GENERATED_BODY()
};

/** Pattern ability of a test launching UGeoTestMontagePattern. */
UCLASS(NotPlaceable, Transient)
class UGeoTestMontagePatternAbility : public UPatternAbility
{
	GENERATED_BODY()

public:
	UGeoTestMontagePatternAbility() { PatternToLaunch = UGeoTestMontagePattern::StaticClass(); }
};

/** Ability of a test that stays active until the test cancels it. Its asset tag (Ability.Type.Basic) is set when
 * granted: native tags do not exist yet in a constructor. */
UCLASS(NotPlaceable, Transient)
class UGeoTestCastAbility : public UGeoGameplayAbility
{
	GENERATED_BODY()

public:
	static FGameplayTag GetCastTag() { return FGeoGameplayTags::Get().Ability_Type_Basic; }

	virtual void ActivateAbility(FGameplayAbilitySpecHandle /*Handle*/, FGameplayAbilityActorInfo const* /*ActorInfo*/,
								 FGameplayAbilityActivationInfo /*ActivationInfo*/,
								 FGameplayEventData const* /*TriggerEventData*/) override
	{
	}

protected:
#if WITH_EDITOR
	virtual void OnGiveAbility(FGameplayAbilityActorInfo const* ActorInfo, FGameplayAbilitySpec const& Spec) override
	{
		EditorGetAssetTags().AddTag(GetCastTag());
		Super::OnGiveAbility(ActorInfo, Spec);
	}
#endif
};

/** Pattern ability of a test launching UGeoTestSilentPattern. */
UCLASS(NotPlaceable, Transient)
class UGeoTestSilentPatternAbility : public UPatternAbility
{
	GENERATED_BODY()

public:
	UGeoTestSilentPatternAbility() { PatternToLaunch = UGeoTestSilentPattern::StaticClass(); }
};
