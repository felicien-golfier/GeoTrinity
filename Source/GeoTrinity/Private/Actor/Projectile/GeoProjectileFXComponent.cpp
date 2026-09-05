// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Projectile/GeoProjectileFXComponent.h"

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "Components/AudioComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Settings/GameDataSettings.h"
#include "Tool/GeoNiagaraParams.h"
#include "Tool/UGeoGameplayLibrary.h"

// Rate the visual slides back onto the actor at (SetVisualLaunchLocation): ~1% of the offset is left after 0.2s, short
// enough that the bullet is on its true path well before anything can be read off its position.
static constexpr float VisualCatchUpSpeed = 25.f;

UGeoProjectileFXComponent::UGeoProjectileFXComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoProjectileFXComponent::TickComponent(float DeltaTime, ELevelTick TickType,
											  FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	FVector const VisualOffset = BulletVFX->GetRelativeLocation();
	if (VisualOffset.IsNearlyZero())
	{
		BulletVFX->SetRelativeLocation(FVector::ZeroVector);
		SetComponentTickEnabled(false);
		return;
	}

	BulletVFX->SetRelativeLocation(
		FMath::VInterpTo(VisualOffset, FVector::ZeroVector, DeltaTime, VisualCatchUpSpeed));
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoProjectileFXComponent::SetPlaybackSubobjects(UNiagaraComponent* const InBulletVFX,
													  UAudioComponent* const InLoopingSound)
{
	BulletVFX = InBulletVFX;
	LoopingSoundComponent = InLoopingSound;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoProjectileFXComponent::ApplyBulletSystem()
{
	if (!DefaultBulletSystem)
	{
		DefaultBulletSystem = BulletVFX->GetAsset();
	}

	FGeoFXMoment const* const Looping = FindMoment(EProjectileMoment::Looping);
	UNiagaraSystem* const DesiredSystem = Looping && Looping->VFX ? Looping->VFX : DefaultBulletSystem;
	// SetAsset restarts the system, so a spawn that keeps the same one must not go through it.
	if (DesiredSystem && BulletVFX->GetAsset() != DesiredSystem)
	{
		BulletVFX->SetAsset(DesiredSystem);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
FGeoFXMoment const* UGeoProjectileFXComponent::FindMoment(EProjectileMoment const Type) const
{
	return GetOwner<AGeoProjectile>()->ResolvedParams.FXMap.Find(Type);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoProjectileFXComponent::ApplyParams()
{
	ApplyBulletSystem();

	FProjectileParamsBase const& Params = GetOwner<AGeoProjectile>()->ResolvedParams;
	BulletVFX->SetVariableFloat(GeoNiagaraParams::BulletRadius, Params.Radius);
	BulletVFX->SetVariableLinearColor(GeoNiagaraParams::BulletHeadColor, Params.HeadColor.GetColor(1.f));
	BulletVFX->SetVariableLinearColor(GeoNiagaraParams::BulletTrailColor, Params.TrailColor.GetColor(1.f));
	BulletVFX->SetVariableFloat(GeoNiagaraParams::TrailLifetimeScale, Params.TrailLifetimeScale);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoProjectileFXComponent::StartLife() const
{
	if (GeoLib::IsDedicatedServer(this))
	{
		return;
	}

	// A pooled instance can come back holding the previous shot's launch offset; the spawner re-applies its own after
	// this runs.
	BulletVFX->SetRelativeLocation(FVector::ZeroVector);
	BulletVFX->Activate(true);

	FGeoFXMoment const* const Looping = FindMoment(EProjectileMoment::Looping);
	if (Looping && !Looping->Sounds.IsEmpty())
	{
		ensureMsgf(Looping->Sounds.Num() == 1,
				   TEXT("%s: %d looping sounds, only the first plays — the projectile owns one audio component"),
				   *GetOwner()->GetName(), Looping->Sounds.Num());
		FGeoSoundEntry const& Entry = Looping->Sounds[0];
		UGeoSoundRowLibrary::ConfigureAudioComponent(LoopingSoundComponent, Entry, GetSoundInstigator(),
													 GetVolume(Entry), GetPitch(Entry));
	}

	if (FGeoFXMoment const* const Start = FindMoment(EProjectileMoment::Start))
	{
		PlayMoment(*Start);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoProjectileFXComponent::PlayEnd(bool const bValidOverlap) const
{
	if (FGeoFXMoment const* const NoOverlapEnd = FindMoment(EProjectileMoment::NoOverlapEnd))
	{
		PlayMoment(*NoOverlapEnd);
	}

	FGeoFXMoment const* const ValidOverlapEnd = FindMoment(EProjectileMoment::ValidOverlapEnd);
	if (bValidOverlap && ValidOverlapEnd)
	{
		PlayMoment(*ValidOverlapEnd);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoProjectileFXComponent::StopAll()
{
	ClearBuffVFX();

	if (GeoLib::IsDedicatedServer(this))
	{
		return;
	}

	LoopingSoundComponent->Stop();
	// Hiding the actor and disabling component ticks does not stop a Niagara system (the world manager ticks it), so a
	// pooled projectile keeps its particles alive and the next reuse renders them for one frame.
	BulletVFX->DeactivateImmediate();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoProjectileFXComponent::SetVisualLaunchLocation(FVector const& WorldLocation)
{
	BulletVFX->SetWorldLocation(WorldLocation);
	SetComponentTickEnabled(true);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoProjectileFXComponent::SetBulletRadius(float const Radius) const
{
	BulletVFX->SetVariableFloat(GeoNiagaraParams::BulletRadius, Radius);
}

// ---------------------------------------------------------------------------------------------------------------------
AActor* UGeoProjectileFXComponent::GetSoundInstigator() const
{
	return GetOwner<AGeoProjectile>()->GetSourceAvatar();
}

// ---------------------------------------------------------------------------------------------------------------------
int32 UGeoProjectileFXComponent::GetAbilityLevel() const
{
	return GetOwner<AGeoProjectile>()->Payload.AbilityLevel;
}

// ---------------------------------------------------------------------------------------------------------------------
UNiagaraSystem* UGeoProjectileFXComponent::GetBuffVFXSystem(FGeoBuffVFXEntry const& Entry) const
{
	UScriptStruct const* ScaledType = nullptr;
	if (Entry.Attribute == UCharacterAttributeSet::GetDamageMultiplierAttribute())
	{
		ScaledType = FDamageEffectData::StaticStruct();
	}
	else if (Entry.Attribute == UCharacterAttributeSet::GetAppliedHealBoostAttribute())
	{
		ScaledType = FHealEffectData::StaticStruct();
	}

	if (!ScaledType || !GeoASLib::HasEffectInArray(GetOwner<AGeoProjectile>()->EffectDataArray, ScaledType))
	{
		return nullptr;
	}
	return UGameDataSettings::GetLoadedDataAsset(Entry.ProjectileVFX);
}
