// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Characters/Component/GeoLoopSoundComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/AudioComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Tool/UGeoGameplayLibrary.h"

UGeoLoopSoundComponent::UGeoLoopSoundComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UGeoLoopSoundComponent::BeginPlay()
{
	Super::BeginPlay();
	Mesh = GetOwner()->FindComponentByClass<USkeletalMeshComponent>();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoLoopSoundComponent::SetPlaying(bool const bPlay, bool const bFade)
{
	if (Loops.IsEmpty() || GeoLib::IsDedicatedServer(this))
	{
		return;
	}

	bPlaying = bPlay;
	if (!bFade)
	{
		Presence = bPlay ? 1.f : 0.f;
	}

	SetComponentTickEnabled(true);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoLoopSoundComponent::TickComponent(float const DeltaTime, ELevelTick const TickType,
										   FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	Presence = FMath::FInterpConstantTo(Presence, bPlaying ? 1.f : 0.f, DeltaTime, 1.f / FadeDuration);
	if (Presence > 0.f)
	{
		if (Running.IsEmpty())
		{
			StartLoops();
		}

		UpdateLoops();
	}
	else
	{
		StopLoops();
		SetComponentTickEnabled(false);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoLoopSoundComponent::StartLoops()
{
	for (FGeoLoopSound const& Loop : Loops)
	{
		FGeoRunningLoopSound& Started = Running.AddDefaulted_GetRef();
		Started.PitchVariation = UGeoSoundRowLibrary::RollPitchVariation(Loop.Sound);
		// Started at the fade's first step, never at zero: a sound spawned silent may be culled before it is heard.
		Started.AudioComponent = UGeoSoundRowLibrary::SpawnAudioComponent(
			GetOwner()->GetRootComponent(), Loop.Sound, GetOwner(),
			Presence * UGeoSoundRowLibrary::GetVolume(Loop.Sound, GetOwner(), 1),
			UGeoSoundRowLibrary::GetPitch(Loop.Sound, GetOwner(), 1, Started.PitchVariation));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoLoopSoundComponent::UpdateLoops() const
{
	float const Time = GetWorld()->GetTimeSeconds();
	float const DriftScale = 1.f - GetMontageWeight();
	for (int32 Index = 0; Index < Running.Num(); ++Index)
	{
		FGeoLoopSound const& Loop = Loops[Index];
		UAudioComponent* const AudioComponent = Running[Index].AudioComponent;
		if (AudioComponent)
		{
			float const Drift = DriftScale * Loop.DriftSemitones
				* FMath::PerlinNoise1D(Time / Loop.DriftPeriod);
			float const Semitones = Drift + GetCurveValue(Loop.PitchCurve);
			float const Decibels = Loop.RestDecibels + GetCurveValue(Loop.VolumeCurve);

			AudioComponent->SetPitchMultiplier(
				UGeoSoundRowLibrary::GetPitch(Loop.Sound, GetOwner(), 1, Running[Index].PitchVariation)
				* FMath::Pow(2.f, Semitones / 12.f));
			AudioComponent->SetVolumeMultiplier(Presence * UGeoSoundRowLibrary::GetVolume(Loop.Sound, GetOwner(), 1)
												* FMath::Pow(10.f, Decibels / 20.f));
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoLoopSoundComponent::StopLoops()
{
	for (FGeoRunningLoopSound const& Stopped : Running)
	{
		if (Stopped.AudioComponent)
		{
			Stopped.AudioComponent->Stop();
			Stopped.AudioComponent->DestroyComponent();
		}
	}

	Running.Empty();
}

// ---------------------------------------------------------------------------------------------------------------------
float UGeoLoopSoundComponent::GetCurveValue(FName const CurveName) const
{
	UAnimInstance const* const AnimInstance = GetAnimInstance();
	return AnimInstance ? AnimInstance->GetCurveValue(CurveName) : 0.f;
}

// ---------------------------------------------------------------------------------------------------------------------
float UGeoLoopSoundComponent::GetMontageWeight() const
{
	UAnimInstance const* const AnimInstance = GetAnimInstance();
	float Weight = 0.f;
	if (AnimInstance)
	{
		for (FAnimMontageInstance const* const MontageInstance : AnimInstance->MontageInstances)
		{
			Weight = FMath::Max(Weight, MontageInstance->GetWeight());
		}
	}

	return Weight;
}

// ---------------------------------------------------------------------------------------------------------------------
UAnimInstance const* UGeoLoopSoundComponent::GetAnimInstance() const
{
	return Mesh ? Mesh->GetAnimInstance() : nullptr;
}
