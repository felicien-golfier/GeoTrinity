// Copyright 2024 GeoTrinity. All Rights Reserved.


#include "AbilitySystem/Data/FX/GeoSoundRow.h"

#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Components/AudioComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Tool/UGeoGameplayLibrary.h"

namespace
{
	/** True when SoundInstigator belongs to a player of another machine: a player's avatar, or anything its abilities
	 * spawned (owned by its PlayerState, the ASC owner, through the actor owner chain). Anything no player owns (a boss
	 * and what it spawns, a drop, a world actor) counts as the instigator machine's, on every machine. */
	bool IsOtherPlayersSound(AActor const* SoundInstigator)
	{
		for (AActor const* Actor = SoundInstigator; Actor; Actor = Actor->GetOwner())
		{
			APawn const* Pawn = Cast<APawn>(Actor);
			if (APlayerState const* PlayerState = Pawn ? Pawn->GetPlayerState() : Cast<APlayerState>(Actor))
			{
				// Another machine's player has no PlayerController here
				APlayerController const* Controller = PlayerState->GetPlayerController();
				return !Controller || !Controller->IsLocalController();
			}
		}

		return false;
	}
} // namespace

bool UGeoSoundRowLibrary::ShouldPlay(UObject const* WorldContextObject, FGeoSoundEntry const& Entry,
									 AActor* SoundInstigator)
{
	if (!IsValid(Entry.Sound) || GeoLib::IsDedicatedServer(WorldContextObject))
	{
		return false;
	}

	if (!IsValid(SoundInstigator))
	{
		return true;
	}

	uint8 const MachineBit = IsOtherPlayersSound(SoundInstigator) ? GeoSoundAudienceMask::OtherMachines
																  : GeoSoundAudienceMask::InstigatorMachine;
	return (Entry.Audience & MachineBit) != 0;
}

// ---------------------------------------------------------------------------------------------------------------------
float UGeoSoundRowLibrary::GetVolume(FGeoSoundEntry const& Entry, AActor* SoundInstigator, int32 const AbilityLevel)
{
	float const Volume =
		IsOtherPlayersSound(SoundInstigator) ? Entry.Volume * Entry.OtherMachinesVolumeMultiplier : Entry.Volume;
	return Volume
		* GeoASLib::SampleAttributeCurve(Entry.VolumeMultiplierCurve, Entry.VolumeAttribute,
										 Entry.bVolumeFromAbilityLevel, SoundInstigator, AbilityLevel);
}

// ---------------------------------------------------------------------------------------------------------------------
float UGeoSoundRowLibrary::GetPitch(FGeoSoundEntry const& Entry, AActor* SoundInstigator, int32 const AbilityLevel,
									float const PitchVariation)
{
	float const Pitch = GeoASLib::SampleAttributeCurve(Entry.PitchCurve, Entry.PitchAttribute,
													   Entry.bPitchFromAbilityLevel, SoundInstigator, AbilityLevel);
	return Pitch * PitchVariation;
}

// ---------------------------------------------------------------------------------------------------------------------
float UGeoSoundRowLibrary::RollPitchVariation(FGeoSoundEntry const& Entry)
{
	return FMath::RandRange(Entry.RandomPitchMultiplierRange.X, Entry.RandomPitchMultiplierRange.Y);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSoundRowLibrary::PlaySoundEntry2D(UObject const* WorldContextObject, FGeoSoundEntry const& Entry,
										   AActor* SoundInstigator, int32 const AbilityLevel)
{
	if (ShouldPlay(WorldContextObject, Entry, SoundInstigator))
	{
		UGameplayStatics::PlaySound2D(WorldContextObject, Entry.Sound, GetVolume(Entry, SoundInstigator, AbilityLevel),
									  GetPitch(Entry, SoundInstigator, AbilityLevel, RollPitchVariation(Entry)),
									  Entry.StartTime);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSoundRowLibrary::PlaySoundEntryAtLocation(UObject const* WorldContextObject, FGeoSoundEntry const& Entry,
												   FVector const& Location, AActor* SoundInstigator,
												   int32 const AbilityLevel)
{
	if (ShouldPlay(WorldContextObject, Entry, SoundInstigator))
	{
		UGameplayStatics::PlaySoundAtLocation(WorldContextObject, Entry.Sound, Location, FRotator::ZeroRotator,
											  GetVolume(Entry, SoundInstigator, AbilityLevel),
											  GetPitch(Entry, SoundInstigator, AbilityLevel, RollPitchVariation(Entry)),
											  Entry.StartTime);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSoundRowLibrary::ConfigureAudioComponent(UAudioComponent* AudioComponent, FGeoSoundEntry const& Entry,
												  AActor* SoundInstigator, float Volume, float Pitch)
{
	if (!ensureMsgf(AudioComponent, TEXT("%hs: null AudioComponent"), __FUNCTION__)
		|| !ShouldPlay(AudioComponent, Entry, SoundInstigator))
	{
		return;
	}

	AudioComponent->SetSound(Entry.Sound);
	AudioComponent->SetVolumeMultiplier(Volume);
	AudioComponent->SetPitchMultiplier(Pitch);
	AudioComponent->Play(Entry.StartTime);
}

// ---------------------------------------------------------------------------------------------------------------------
UAudioComponent* UGeoSoundRowLibrary::SpawnAudioComponent(USceneComponent* AttachTo, FGeoSoundEntry const& Entry,
														  AActor* SoundInstigator, float Volume, float Pitch)
{
	if (!ensureMsgf(AttachTo, TEXT("%hs: null AttachTo"), __FUNCTION__)
		|| !ShouldPlay(AttachTo, Entry, SoundInstigator))
	{
		return nullptr;
	}

	return UGameplayStatics::SpawnSoundAttached(Entry.Sound, AttachTo, NAME_None, FVector::ZeroVector,
												EAttachLocation::SnapToTarget, /*bStopWhenAttachedToDestroyed*/ true,
												Volume, Pitch, Entry.StartTime, nullptr, nullptr,
												/*bAutoDestroy*/ false);
}
