// Copyright 2024 GeoTrinity. All Rights Reserved.


#include "AbilitySystem/Data/GeoSoundRow.h"

#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Curves/CurveFloat.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Tool/UGeoGameplayLibrary.h"

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
float UGeoSoundRowLibrary::GetVolume(FGeoSoundEntry const& Entry, AActor* SoundInstigator)
{
	bool const bInstigatorMachine = !IsValid(SoundInstigator) || GeoLib::IsLocalPlayerAvatar(SoundInstigator);
	return bInstigatorMachine ? Entry.Volume : Entry.Volume * Entry.OtherMachinesVolumeMultiplier;
}

// ---------------------------------------------------------------------------------------------------------------------
float UGeoSoundRowLibrary::GetPitch(FGeoSoundEntry const& Entry, AActor* SoundInstigator)
{
	float Pitch = 1.f;

	if (IsValid(Entry.AttributeToPitchCurve) && Entry.PitchAttribute.IsValid() && IsValid(SoundInstigator))
	{
		if (UGeoAbilitySystemComponent* ASC = GeoASLib::GetGeoAscFromActor(SoundInstigator); IsValid(ASC))
		{
			bool bFound = false;
			float const AttributeValue = ASC->GetGameplayAttributeValue(Entry.PitchAttribute, bFound);
			if (bFound)
			{
				Pitch = Entry.AttributeToPitchCurve->GetFloatValue(AttributeValue);
			}
		}
	}

	Pitch *= FMath::RandRange(Entry.RandomPitchMultiplierRange.X, Entry.RandomPitchMultiplierRange.Y);

	return Pitch;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSoundRowLibrary::PlaySoundEntry2D(UObject const* WorldContextObject, FGeoSoundEntry const& Entry,
										   AActor* SoundInstigator)
{
	if (ShouldPlay(WorldContextObject, Entry, SoundInstigator))
	{
		UGameplayStatics::PlaySound2D(WorldContextObject, Entry.Sound, GetVolume(Entry, SoundInstigator),
									  GetPitch(Entry, SoundInstigator), Entry.StartTime);
	}
}
