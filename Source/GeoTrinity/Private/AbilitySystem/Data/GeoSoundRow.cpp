// Copyright 2024 GeoTrinity. All Rights Reserved.


#include "AbilitySystem/Data/GeoSoundRow.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Components/AudioComponent.h"
#include "Curves/CurveFloat.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Tool/UGeoGameplayLibrary.h"

// ---------------------------------------------------------------------------------------------------------------------
FGeoSoundRow UGeoSoundRowLibrary::FindSoundForTag(UDataTable const* SoundTable, FGameplayTag Tag, bool& bFound)
{
	bFound = false;
	FGeoSoundRow FoundRow;
	if (!ensureMsgf(SoundTable, TEXT("FindSoundForTag called with null SoundTable")))
	{
		return FoundRow;
	}

	SoundTable->ForeachRow<FGeoSoundRow>(TEXT("FindSoundForTag"),
										 [&Tag, &FoundRow, &bFound](FName const& /*RowName*/, FGeoSoundRow const& Row)
										 {
											 if (!bFound && Row.Tag.MatchesTagExact(Tag))
											 {
												 bFound = true;
												 FoundRow = Row;
											 }
										 });
	return FoundRow;
}

// ---------------------------------------------------------------------------------------------------------------------
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
		* SampleSourceCurve(Entry.VolumeMultiplierCurve, Entry.VolumeAttribute, Entry.bVolumeFromAbilityLevel,
							SoundInstigator, AbilityLevel);
}

// ---------------------------------------------------------------------------------------------------------------------
float UGeoSoundRowLibrary::GetPitch(FGeoSoundEntry const& Entry, AActor* SoundInstigator, int32 const AbilityLevel)
{
	float const Pitch = SampleSourceCurve(Entry.PitchCurve, Entry.PitchAttribute, Entry.bPitchFromAbilityLevel,
										  SoundInstigator, AbilityLevel);
	return Pitch * FMath::RandRange(Entry.RandomPitchMultiplierRange.X, Entry.RandomPitchMultiplierRange.Y);
}

// ---------------------------------------------------------------------------------------------------------------------
float UGeoSoundRowLibrary::SampleSourceCurve(UCurveFloat const* Curve, FGameplayAttribute const& Attribute,
											 bool const bFromAbilityLevel, AActor* SoundInstigator,
											 int32 const AbilityLevel)
{
	if (!IsValid(Curve))
	{
		return 1.f;
	}

	if (bFromAbilityLevel)
	{
		return Curve->GetFloatValue(AbilityLevel);
	}

	UGeoAbilitySystemComponent* const ASC = GeoASLib::GetGeoAscFromActor(SoundInstigator);
	if (!Attribute.IsValid() || !IsValid(ASC))
	{
		return 1.f;
	}

	bool bFound = false;
	float const AttributeValue = ASC->GetGameplayAttributeValue(Attribute, bFound);
	return bFound ? Curve->GetFloatValue(AttributeValue) : 1.f;
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
