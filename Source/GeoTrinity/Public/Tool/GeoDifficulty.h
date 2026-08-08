// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "GeoDifficulty.generated.h"

/**
 * The three tunings one boss encounter ships with. The bit value doubles as the ability level: AGeoArena stamps it on
 * its boss's ASC, which levels every ability spec, the DefaultAttributes effect and every FAbilityPayload with it — so
 * lowering the difficulty walks the whole kit down its FScalableFloat curves at once. An FScalableFloat with no curve
 * returns its constant at any level, so a knob nobody tuned behaves identically on all three.
 *
 * Declared as flags so FEffectData can gate an entry on a set of difficulties (lethal hits stay out of Safe) with the
 * same numbers the level uses; a single-value property of this type still edits as a plain dropdown.
 */
UENUM(Meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EGeoDifficulty : uint8
{
	Safe = (1 << 0) UMETA(DisplayName = "Safe"),
	Reduced = (1 << 1) UMETA(DisplayName = "Reduced"),
	Original = (1 << 2) UMETA(DisplayName = "Original")
};
ENUM_CLASS_FLAGS(EGeoDifficulty)

namespace GeoDifficultyMask
{
	constexpr int32 Safe = static_cast<int32>(EGeoDifficulty::Safe);
	constexpr int32 Reduced = static_cast<int32>(EGeoDifficulty::Reduced);
	constexpr int32 Original = static_cast<int32>(EGeoDifficulty::Original);
	constexpr int32 All = Safe | Reduced | Original;
} // namespace GeoDifficultyMask
