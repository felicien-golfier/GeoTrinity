// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Characters/Component/GeoFXComponent.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Data/GeoFXMoment.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Settings/GameDataSettings.h"
#include "Tool/GeoNiagaraParams.h"
#include "Tool/UGeoGameplayLibrary.h"

void UGeoFXComponent::PlayMoment(FGeoFXMoment const& Moment) const
{
	if (GeoLib::IsDedicatedServer(this))
	{
		return;
	}

	// Spawned inactive: a User parameter has to be there before the first tick reads it. Niagara returns nothing when
	// the moment carries no system, and when it pre-culls the spawn.
	if (UNiagaraComponent* const Spawned = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this, Moment.VFX, GetOwner()->GetActorLocation(), FRotator::ZeroRotator, FVector::OneVector,
			/*bAutoDestroy*/ true, /*bAutoActivate*/ false))
	{
		float const NormalizedMagnitude = FMath::Clamp(
			GeoASLib::SampleAttributeCurve(Moment.MagnitudeCurve, Moment.MagnitudeAttribute,
										   Moment.bMagnitudeFromAbilityLevel, GetFXInstigator(), GetAbilityLevel()),
			0.f, 1.f);

		Spawned->SetVariableLinearColor(GeoNiagaraParams::Color, Moment.Color.GetColor());
		Spawned->SetVariableFloat(GeoNiagaraParams::NormalizedMagnitude, NormalizedMagnitude);
		if (Moment.Radius > 0.f)
		{
			Spawned->SetVariableFloat(GeoNiagaraParams::Radius, Moment.Radius);
		}
		if (Moment.Lifetime > 0.f)
		{
			Spawned->SetVariableFloat(GeoNiagaraParams::Lifetime, Moment.Lifetime);
		}
		Spawned->Activate();
	}

	for (FGeoSoundEntry const& Entry : Moment.Sounds)
	{
		PlaySound(Entry);
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
void UGeoFXComponent::SetAttachedVFX(UNiagaraSystem* const System, bool const bShow)
{
	if (!System || GeoLib::IsDedicatedServer(this))
	{
		return;
	}

	int32 const Index = AttachedVFXComponents.IndexOfByPredicate(
		[System](UNiagaraComponent const* const AttachedVFX)
		{
			return AttachedVFX->GetAsset() == System;
		});

	if (bShow == (Index != INDEX_NONE))
	{
		return;
	}

	if (!bShow)
	{
		AttachedVFXComponents[Index]->DestroyComponent();
		AttachedVFXComponents.RemoveAtSwap(Index);
		return;
	}

	// Niagara returns nothing when it pre-culls the spawn.
	if (UNiagaraComponent* const Spawned = UNiagaraFunctionLibrary::SpawnSystemAttached(
			System, GetOwner()->GetRootComponent(), NAME_None, FVector::ZeroVector, FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget, false))
	{
		AttachedVFXComponents.Add(Spawned);
	}
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
UNiagaraSystem* UGeoFXComponent::GetBuffVFXSystem(FGeoBuffVFXEntry const& Entry) const
{
	return UGameDataSettings::GetLoadedDataAsset(Entry.CharacterVFX);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoFXComponent::BindBuffVFX(UGeoAbilitySystemComponent* const SourceASC)
{
	if (GeoLib::IsDedicatedServer(this) || !IsValid(SourceASC))
	{
		return;
	}

	if (BuffSourceASC != SourceASC)
	{
		ClearBuffVFX();
		BuffSourceASC = SourceASC;

		for (FGeoBuffVFXEntry const& Entry : GetDefault<UGameDataSettings>()->BuffVFX)
		{
			SourceASC->GetGameplayAttributeValueChangeDelegate(Entry.Attribute)
				.AddWeakLambda(this,
							   [this](FOnAttributeChangeData const& /*Data*/)
							   {
								   RefreshBuffVFX();
							   });
		}
	}

	RefreshBuffVFX();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoFXComponent::RefreshBuffVFX()
{
	UGeoAbilitySystemComponent const* const SourceASC = BuffSourceASC.Get();
	if (!SourceASC)
	{
		return;
	}

	for (FGeoBuffVFXEntry const& Entry : GetDefault<UGameDataSettings>()->BuffVFX)
	{
		SetAttachedVFX(GetBuffVFXSystem(Entry), GeoASLib::IsBuffed(*SourceASC, Entry.Attribute));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoFXComponent::ClearBuffVFX()
{
	if (UGeoAbilitySystemComponent* const SourceASC = BuffSourceASC.Get())
	{
		for (FGeoBuffVFXEntry const& Entry : GetDefault<UGameDataSettings>()->BuffVFX)
		{
			SourceASC->GetGameplayAttributeValueChangeDelegate(Entry.Attribute).RemoveAll(this);
		}
	}
	BuffSourceASC = nullptr;

	for (UNiagaraComponent* const AttachedVFX : AttachedVFXComponents)
	{
		AttachedVFX->DestroyComponent();
	}
	AttachedVFXComponents.Empty();
}
