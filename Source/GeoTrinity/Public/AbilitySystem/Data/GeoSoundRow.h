// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AttributeSet.h"
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Serialization/StructuredArchiveFwd.h"

#include "GeoSoundRow.generated.h"


class UAudioComponent;
class USoundBase;
class UCurveFloat;
struct FPropertyTag;

UENUM(Meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EGeoSoundAudienceBitflag : uint8
{
	InstigatorMachine = (1 << 0) UMETA(DisplayName = "Instigator's Machine"),
	OtherMachines = (1 << 1) UMETA(DisplayName = "Other Machines"),
};
ENUM_CLASS_FLAGS(EGeoSoundAudienceBitflag)

namespace GeoSoundAudienceMask
{
	constexpr uint8 InstigatorMachine = static_cast<uint8>(EGeoSoundAudienceBitflag::InstigatorMachine);
	constexpr uint8 OtherMachines = static_cast<uint8>(EGeoSoundAudienceBitflag::OtherMachines);
	constexpr uint8 All = InstigatorMachine | OtherMachines;
} // namespace GeoSoundAudienceMask

/**
 * A gameplay sound: the asset, its volume, which machines it plays on (relative to the sound's instigator),
 * and curve-driven volume and pitch modifiers.
 * Volume and pitch each carry their own curve, sampled at either an instigator attribute value or the sound's ability
 * level: the volume curve scales Volume, the pitch curve gives the pitch, which is then multiplied by a random value
 * in RandomPitchMultiplierRange.
 * Play it through UGeoSoundRowLibrary so audience gating and volume/pitch rules apply everywhere.
 */
USTRUCT(BlueprintType)
struct FGeoSoundEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USoundBase> Sound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0"))
	float Volume = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0"))
	float StartTime = 0.f;

	/** Machines the sound plays on, relative to the sound's instigator. When no instigator is provided at play time,
	 * the sound plays regardless of this mask. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly,
			  meta = (Bitmask, BitmaskEnum = "/Script/GeoTrinity.EGeoSoundAudienceBitflag"))
	int32 Audience = GeoSoundAudienceMask::All;

	/** Volume multiplier applied on machines where the instigator is not the local player's avatar, so other players'
	 * sounds are quieter than your own. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0"))
	float OtherMachinesVolumeMultiplier = 0.5f;

	/** Instigator attribute VolumeMultiplierCurve is sampled at. Hidden while bVolumeFromAbilityLevel is set —
	 * one source or the other, never both. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayAttribute VolumeAttribute;

	/** Samples VolumeMultiplierCurve at the ability level the sound was fired with instead of at an attribute
	 * value. Hidden while VolumeAttribute is set, for the same reason. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bVolumeFromAbilityLevel = false;

	/** Scales Volume by its value at the chosen source — VolumeAttribute's value or the ability level. Hidden until
	 * one of the two is chosen, since it has nothing to sample against otherwise. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UCurveFloat> VolumeMultiplierCurve;

	/** Instigator attribute PitchCurve is sampled at. Hidden while bPitchFromAbilityLevel is set — a pitch reads from
	 * one source or the other, never both. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayAttribute PitchAttribute;

	/** Samples PitchCurve at the ability level the sound was fired with instead of at an attribute value. Hidden while
	 * PitchAttribute is set, for the same reason. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bPitchFromAbilityLevel = false;

	/** Maps the chosen source — PitchAttribute's value or the ability level — to a pitch multiplier. Hidden
	 * until one of the two sources is chosen, since it has nothing to sample against otherwise. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UCurveFloat> PitchCurve;

	/** Random pitch multiplier range applied on top of the curve result. X = min, Y = max. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0"))
	FVector2D RandomPitchMultiplierRange = FVector2D(1.f, 1.f);
};

/**
 * The sounds one moment plays: every entry fires together, so a moment can layer several assets.
 * A wrapper struct because a UPROPERTY TMap cannot hold a TArray as its value.
 */
USTRUCT(BlueprintType)
struct FGeoSoundEntryList
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FGeoSoundEntry> Entries;

	/** Loads an asset saved when this property held a single FGeoSoundEntry, turning it into a one-element
	 * Entries. Temporary migration hook — delete once every deployable asset has been resaved. */
	bool SerializeFromMismatchedTag(FPropertyTag const& Tag, FStructuredArchive::FSlot Slot);
};

template <>
struct TStructOpsTypeTraits<FGeoSoundEntryList> : public TStructOpsTypeTraitsBase2<FGeoSoundEntryList>
{
	enum
	{
		WithStructuredSerializeFromMismatchedTag = true,
	};
};

/** DataTable row mapping a sound tag to the FGeoSoundEntry to play. The tag is an explicit field, not the row name. */
USTRUCT(BlueprintType)
struct FGeoSoundRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Event.Sound"))
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGeoSoundEntry Entry;
};

/**
 * Static helpers for playing FGeoSoundEntry sounds. The only valid playback path for gameplay sounds in the
 * project — never call PlaySoundAtLocation or PlaySound2D directly. Centralises audience gating,
 * dedicated-server suppression, instigator-relative volume, and curve-driven pitch so every call site
 * behaves consistently.
 */
UCLASS()
class GEOTRINITY_API UGeoSoundRowLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Returns the first row whose Tag matches Tag; bFound is false and a default row is returned if none. */
	UFUNCTION(BlueprintPure, Category = "GeoTrinity|Sound", meta = (DataTablePin = "SoundTable"))
	static FGeoSoundRow FindSoundForTag(UDataTable const* SoundTable, FGameplayTag Tag, bool& bFound);

	/** Returns true when Entry should play on this machine: never on a dedicated server or without a valid Sound;
	 * otherwise gated by Entry.Audience relative to SoundInstigator (always plays when SoundInstigator is null). */
	static bool ShouldPlay(UObject const* WorldContextObject, FGeoSoundEntry const& Entry, AActor* SoundInstigator);

	/** Returns Entry.Volume scaled by VolumeMultiplierCurve at its chosen source, and multiplied by
	 * Entry.OtherMachinesVolumeMultiplier when SoundInstigator is not the local player's avatar. */
	static float GetVolume(FGeoSoundEntry const& Entry, AActor* SoundInstigator, int32 AbilityLevel);

	/**
	 * Returns the pitch for Entry: PitchCurve sampled at AbilityLevel when bPitchFromAbilityLevel is set or at
	 * SoundInstigator's PitchAttribute value otherwise, multiplied by a random value in RandomPitchMultiplierRange.
	 */
	static float GetPitch(FGeoSoundEntry const& Entry, AActor* SoundInstigator, int32 AbilityLevel);

	/** Plays Entry once as a 2D sound, applying ShouldPlay gating and instigator-relative volume and pitch. */
	UFUNCTION(BlueprintCallable, Category = "GeoTrinity|Sound", meta = (DefaultToSelf = "WorldContextObject"))
	static void PlaySoundEntry2D(UObject const* WorldContextObject, FGeoSoundEntry const& Entry,
								 AActor* SoundInstigator, int32 AbilityLevel = 1);

	/** Sets Entry's sound, Volume and Pitch on AudioComponent and starts it — the looping counterpart of
	 * PlaySoundEntry2D, applying the same ShouldPlay gating. Does nothing when the entry must not play here.
	 * Volume and Pitch are parameters so callers owning their own hooks (AGeoProjectile::GetVolume/GetPitch) keep
	 * them; pass GetVolume/GetPitch(Entry, SoundInstigator, AbilityLevel) otherwise. */
	static void ConfigureAudioComponent(UAudioComponent* AudioComponent, FGeoSoundEntry const& Entry,
										AActor* SoundInstigator, float Volume, float Pitch);

private:
	/** Returns Curve's value at the source the sound reads from — AbilityLevel when bFromAbilityLevel, otherwise
	 * Attribute's value on SoundInstigator's ASC. Returns 1 when there is no curve, or no source to sample. */
	static float SampleSourceCurve(UCurveFloat const* Curve, FGameplayAttribute const& Attribute,
								   bool bFromAbilityLevel, AActor* SoundInstigator, int32 AbilityLevel);
};
