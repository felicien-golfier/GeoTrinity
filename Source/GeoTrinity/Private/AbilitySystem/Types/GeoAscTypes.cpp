// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Types/GeoAscTypes.h"

namespace
{
	/**
	 * Bit positions used by NetSerialize to pack optional fields into a single uint32.
	 * Each field is only written/read when its corresponding bit is set, keeping bandwidth minimal.
	 * The order must remain stable across builds — never reorder or remove existing values.
	 */
	enum RepFlag
	{
		REP_Icon,
		REP_MAX
	};
} // namespace

FGeoGameplayEffectContext* FGeoGameplayEffectContext::Duplicate() const
{
	FGeoGameplayEffectContext* NewContext = new FGeoGameplayEffectContext();
	*NewContext = *this;
	if (GetHitResult())
	{
		// Does a deep copy of the hit result
		NewContext->AddHitResult(*GetHitResult(), true);
	}
	return NewContext;
}

bool FGeoGameplayEffectContext::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	FGameplayEffectContext::NetSerialize(Ar, Map, bOutSuccess);

	uint32 RepBits = 0;

	if (Ar.IsSaving() && Icon)
	{
		RepBits |= 1 << REP_Icon;
	}

	Ar.SerializeBits(&RepBits, REP_MAX);

	// Serialize StatusTag in both directions
	Ar << StatusTag;

	// Serialization in both cases (check RepBits to see whether we need to save data, or read RepBits to see what's in
	// the archive to load)
	if (RepBits & (1 << REP_Icon))
	{
		Ar << Icon;
	}

	bOutSuccess = true;
	return true;
}
