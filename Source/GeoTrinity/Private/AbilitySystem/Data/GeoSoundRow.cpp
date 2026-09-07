// Copyright 2024 GeoTrinity. All Rights Reserved.


#include "AbilitySystem/Data/GeoSoundRow.h"

#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Tool/UGeoGameplayLibrary.h"

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

	uint8 const MachineBit = GeoLib::IsLocalPlayerAvatar(SoundInstigator) ? GeoSoundAudienceMask::InstigatorMachine
																		  : GeoSoundAudienceMask::OtherMachines;
	return (Entry.Audience & MachineBit) != 0;
}

// ---------------------------------------------------------------------------------------------------------------------
float UGeoSoundRowLibrary::GetVolume(FGeoSoundEntry const& Entry, AActor* SoundInstigator, int32 const AbilityLevel)
{
	bool const bInstigatorMachine = !IsValid(SoundInstigator) || GeoLib::IsLocalPlayerAvatar(SoundInstigator);
	float const Volume = bInstigatorMachine ? Entry.Volume : Entry.Volume * Entry.OtherMachinesVolumeMultiplier;
	return Volume
		* GeoASLib::SampleAttributeCurve(Entry.VolumeMultiplierCurve, Entry.VolumeAttribute,
										 Entry.bVolumeFromAbilityLevel, SoundInstigator, AbilityLevel);
}

// ---------------------------------------------------------------------------------------------------------------------
float UGeoSoundRowLibrary::GetPitch(FGeoSoundEntry const& Entry, AActor* SoundInstigator, int32 const AbilityLevel)
{
	float const Pitch = GeoASLib::SampleAttributeCurve(Entry.PitchCurve, Entry.PitchAttribute,
													   Entry.bPitchFromAbilityLevel, SoundInstigator, AbilityLevel);
	return Pitch * FMath::RandRange(Entry.RandomPitchMultiplierRange.X, Entry.RandomPitchMultiplierRange.Y);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSoundRowLibrary::PlaySoundEntry2D(UObject const* WorldContextObject, FGeoSoundEntry const& Entry,
										   AActor* SoundInstigator, int32 const AbilityLevel)
{
	if (ShouldPlay(WorldContextObject, Entry, SoundInstigator))
	{
		UGameplayStatics::PlaySound2D(WorldContextObject, Entry.Sound, GetVolume(Entry, SoundInstigator, AbilityLevel),
									  GetPitch(Entry, SoundInstigator, AbilityLevel), Entry.StartTime);
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
