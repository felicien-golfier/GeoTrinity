// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Characters/Component/GeoFXComponent.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Data/GeoBuffFXDataAsset.h"
#include "AbilitySystem/Data/GeoFXMoment.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Settings/GameDataSettings.h"
#include "Tool/GeoNiagaraParams.h"
#include "Tool/UGeoGameplayLibrary.h"

void FGeoRunningSustainedFX::Stop() const
{
	VFXComponent->DestroyComponent();
	if (AudioComponent)
	{
		AudioComponent->Stop();
		AudioComponent->DestroyComponent();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoFXComponent::PlayBurst(FGeoBurstFXMoment const& Moment) const
{
	if (GeoLib::IsDedicatedServer(this))
	{
		return;
	}

	// Spawned inactive: a User parameter has to be there before the first tick reads it. Niagara returns nothing when
	// the moment carries no system, and when it pre-culls the spawn.
	if (UNiagaraComponent* const Spawned = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this, Moment.VFX.System, GetOwner()->GetActorLocation(), FRotator::ZeroRotator, FVector::OneVector,
			/*bAutoDestroy*/ true, /*bAutoActivate*/ false))
	{
		ApplyFXParams(Spawned, Moment.VFX);
		if (Moment.VFX.Lifetime > 0.f)
		{
			Spawned->SetVariableFloat(GeoNiagaraParams::Lifetime, Moment.VFX.Lifetime);
		}
		Spawned->Activate();
	}

	for (FGeoSoundEntry const& Entry : Moment.Sounds)
	{
		PlaySound(Entry);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoFXComponent::SetSustainedFX(FGeoSustainedFXMoment const& Moment, bool const bShow)
{
	if (!Moment.VFX.System || GeoLib::IsDedicatedServer(this))
	{
		return;
	}

	int32 const Index = RunningSustainedFX.IndexOfByPredicate(
		[&Moment](FGeoRunningSustainedFX const& Running)
		{
			return Running.VFXComponent->GetAsset() == Moment.VFX.System;
		});

	if (!bShow)
	{
		if (Index != INDEX_NONE)
		{
			RunningSustainedFX[Index].Stop();
			RunningSustainedFX.RemoveAtSwap(Index);
		}
		return;
	}

	if (Index != INDEX_NONE)
	{
		ApplyFXParams(RunningSustainedFX[Index].VFXComponent, Moment.VFX);
		return;
	}

	// Spawned inactive for the same reason a burst is; Niagara returns nothing when it pre-culls the spawn.
	UNiagaraComponent* const Spawned = UNiagaraFunctionLibrary::SpawnSystemAttached(
		Moment.VFX.System, GetOwner()->GetRootComponent(), NAME_None, FVector::ZeroVector, FRotator::ZeroRotator,
		EAttachLocation::SnapToTarget, /*bAutoDestroy*/ false, /*bAutoActivate*/ false);
	if (!Spawned)
	{
		return;
	}

	ApplyFXParams(Spawned, Moment.VFX);
	Spawned->Activate();

	FGeoRunningSustainedFX& Running = RunningSustainedFX.AddDefaulted_GetRef();
	Running.VFXComponent = Spawned;
	Running.AudioComponent =
		UGeoSoundRowLibrary::SpawnAudioComponent(GetOwner()->GetRootComponent(), Moment.Sound, GetFXInstigator(),
												 GetVolume(Moment.Sound), GetPitch(Moment.Sound));
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoFXComponent::ApplyFXParams(UNiagaraComponent* const Component, FGeoVFXParams const& Params) const
{
	float const NormalizedMagnitude = FMath::Clamp(
		GeoASLib::SampleAttributeCurve(Params.MagnitudeCurve, Params.MagnitudeAttribute,
									   Params.bMagnitudeFromAbilityLevel, GetFXInstigator(), GetAbilityLevel()),
		0.f, 1.f);

	Component->SetVariableLinearColor(GeoNiagaraParams::Color, Params.Color.GetColor());
	Component->SetVariableFloat(GeoNiagaraParams::NormalizedMagnitude, NormalizedMagnitude);
	if (Params.RadiusOverride > 0.f)
	{
		Component->SetVariableFloat(GeoNiagaraParams::Radius, Params.RadiusOverride);
	}
	else
	{
		Component->SetVariableFloat(GeoNiagaraParams::Radius, GetFXInstigator()->GetSimpleCollisionRadius())
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoFXComponent::PlaySound(FGeoSoundEntry const& Entry) const
{
	if (UGeoSoundRowLibrary::ShouldPlay(this, Entry, GetFXInstigator()))
	{
		UGameplayStatics::PlaySoundAtLocation(this, Entry.Sound, GetOwner()->GetActorLocation(), FRotator::ZeroRotator,
											  GetVolume(Entry), GetPitch(Entry), Entry.StartTime);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
float UGeoFXComponent::GetVolume(FGeoSoundEntry const& Entry) const
{
	return UGeoSoundRowLibrary::GetVolume(Entry, GetFXInstigator(), GetAbilityLevel());
}

// ---------------------------------------------------------------------------------------------------------------------
float UGeoFXComponent::GetPitch(FGeoSoundEntry const& Entry) const
{
	return UGeoSoundRowLibrary::GetPitch(Entry, GetFXInstigator(), GetAbilityLevel()) * PitchMultiplier;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoFXComponent::SetPitchMultiplier(float const Multiplier)
{
	PitchMultiplier = Multiplier;
}

// ---------------------------------------------------------------------------------------------------------------------
AActor* UGeoFXComponent::GetFXInstigator() const
{
	return GetOwner();
}

// ---------------------------------------------------------------------------------------------------------------------
int32 UGeoFXComponent::GetAbilityLevel() const
{
	return 1;
}

// ---------------------------------------------------------------------------------------------------------------------
FGeoSustainedFXMoment const* UGeoFXComponent::GetBuffMoment(FGeoBuffFXEntry const& Entry) const
{
	return &Entry.CharacterFX;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoFXComponent::BindBuffFX(UGeoAbilitySystemComponent* const SourceASC)
{
	if (GeoLib::IsDedicatedServer(this) || !IsValid(SourceASC))
	{
		return;
	}

	if (BuffSourceASC != SourceASC)
	{
		ClearBuffFX();
		BuffSourceASC = SourceASC;

		for (FGeoBuffFXEntry const& Entry : GetBuffEntries())
		{
			SourceASC->GetGameplayAttributeValueChangeDelegate(Entry.Attribute)
				.AddWeakLambda(this,
							   [this](FOnAttributeChangeData const& /*Data*/)
							   {
								   RefreshBuffFX();
							   });
		}
	}

	RefreshBuffFX();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoFXComponent::RefreshBuffFX()
{
	UGeoAbilitySystemComponent const* const SourceASC = BuffSourceASC.Get();
	if (!SourceASC)
	{
		return;
	}

	for (FGeoBuffFXEntry const& Entry : GetBuffEntries())
	{
		if (FGeoSustainedFXMoment const* const Moment = GetBuffMoment(Entry))
		{
			SetSustainedFX(*Moment, GeoASLib::IsBuffed(*SourceASC, Entry.Attribute));
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoFXComponent::ClearBuffFX()
{
	if (UGeoAbilitySystemComponent* const SourceASC = BuffSourceASC.Get())
	{
		for (FGeoBuffFXEntry const& Entry : GetBuffEntries())
		{
			SourceASC->GetGameplayAttributeValueChangeDelegate(Entry.Attribute).RemoveAll(this);
		}
	}
	BuffSourceASC = nullptr;

	for (FGeoRunningSustainedFX const& Running : RunningSustainedFX)
	{
		Running.Stop();
	}
	RunningSustainedFX.Empty();
}

// ---------------------------------------------------------------------------------------------------------------------
TArray<FGeoBuffFXEntry> const& UGeoFXComponent::GetBuffEntries()
{
	static TArray<FGeoBuffFXEntry> const NoBuffFXConfigured;

	UGeoBuffFXDataAsset const* const Asset =
		UGameDataSettings::GetLoadedDataAsset(GetDefault<UGameDataSettings>()->BuffFX);
	return Asset ? Asset->Entries : NoBuffFXConfigured;
}
