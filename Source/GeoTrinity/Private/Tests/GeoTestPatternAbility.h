// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/PatternAbility.h"
#include "AbilitySystem/Abilities/Pattern/Pattern.h"
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

/** Pattern ability of a test launching UGeoTestSilentPattern. */
UCLASS(NotPlaceable, Transient)
class UGeoTestSilentPatternAbility : public UPatternAbility
{
	GENERATED_BODY()

public:
	UGeoTestSilentPatternAbility() { PatternToLaunch = UGeoTestSilentPattern::StaticClass(); }
};
