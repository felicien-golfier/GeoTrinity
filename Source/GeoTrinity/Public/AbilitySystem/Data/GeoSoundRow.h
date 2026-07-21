// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AttributeSet.h"
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "GeoSoundRow.generated.h"


class USoundBase;
class UCurveFloat;

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
 * and an attribute-driven pitch modifier with optional random variance.
 * The base pitch is read from the instigator's ASC attribute value sampled against AttributeToPitchCurve;
 * the result is then multiplied by a random value in RandomPitchMultiplierRange.
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayAttribute PitchAttribute;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UCurveFloat> AttributeToPitchCurve;

	/** Random pitch multiplier range applied on top of the curve result. X = min, Y = max. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0"))
	FVector2D RandomPitchMultiplierRange = FVector2D(1.f, 1.f);
};

/** DataTable row mapping a sound tag to the FGeoSoundEntry to play. The tag is an explicit field, not the row name. */
USTRUCT(BlueprintType)
struct FGeoSoundRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGeoSoundEntry Entry;
};

/**
 * Static helpers for playing FGeoSoundEntry sounds. The only valid playback path for gameplay sounds in the
 * project — never call PlaySoundAtLocation or PlaySound2D directly. Centralises audience gating,
 * dedicated-server suppression, instigator-relative volume, and attribute-driven pitch so every call site
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

	/** Returns Entry.Volume, multiplied by Entry.OtherMachinesVolumeMultiplier when SoundInstigator is not the local
	 * player's avatar. */
	static float GetVolume(FGeoSoundEntry const& Entry, AActor* SoundInstigator);

	/** Returns the pitch for Entry: SoundInstigator's PitchAttribute value sampled against AttributeToPitchCurve, multiplied by a
	 * random value in RandomPitchMultiplierRange. */
	static float GetPitch(FGeoSoundEntry const& Entry, AActor* SoundInstigator);

	/** Plays Entry once as a 2D sound, applying ShouldPlay gating and instigator-relative volume and pitch. */
	UFUNCTION(BlueprintCallable, Category = "GeoTrinity|Sound", meta = (DefaultToSelf = "WorldContextObject"))
	static void PlaySoundEntry2D(UObject const* WorldContextObject, FGeoSoundEntry const& Entry,
								 AActor* SoundInstigator);
};
