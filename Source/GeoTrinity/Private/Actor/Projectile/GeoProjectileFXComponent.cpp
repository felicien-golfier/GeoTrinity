// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Projectile/GeoProjectileFXComponent.h"

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystem/Data/GeoBuffFXDataAsset.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "Components/AudioComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"
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

	FVector const VisualOffset = Flight.VFXComponent->GetRelativeLocation();
	if (VisualOffset.IsNearlyZero())
	{
		Flight.VFXComponent->SetRelativeLocation(FVector::ZeroVector);
		SetComponentTickEnabled(false);
		return;
	}

	Flight.VFXComponent->SetRelativeLocation(
		FMath::VInterpTo(VisualOffset, FVector::ZeroVector, DeltaTime, VisualCatchUpSpeed));
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoProjectileFXComponent::SetPlaybackSubobjects(UNiagaraComponent* const InBulletVFX,
													  UAudioComponent* const InLoopingSound)
{
	Flight.VFXComponent = InBulletVFX;
	Flight.AudioComponent = InLoopingSound;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoProjectileFXComponent::ApplyBulletSystem()
{
	if (!DefaultBulletSystem)
	{
		DefaultBulletSystem = Flight.VFXComponent->GetAsset();
	}

	UNiagaraSystem* const LoopingSystem = GetOwner<AGeoProjectile>()->ResolvedParams.LoopingFX.VFX.System;
	UNiagaraSystem* const DesiredSystem = LoopingSystem ? LoopingSystem : DefaultBulletSystem.Get();
	// SetAsset restarts the system, so a spawn that keeps the same one must not go through it.
	if (DesiredSystem && Flight.VFXComponent->GetAsset() != DesiredSystem)
	{
		Flight.VFXComponent->SetAsset(DesiredSystem);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoProjectileFXComponent::PlayMoment(EProjectileMoment const Type) const
{
	if (FGeoBurstFXMoment const* const Moment = GetOwner<AGeoProjectile>()->ResolvedParams.FXMap.Find(Type))
	{
		PlayBurst(*Moment);
	}

	for (FGeoBuffFXEntry const& Entry : GetBuffEntries())
	{
		FGeoBurstFXMoment const* const BuffMoment = Entry.ProjectileBurstFX.Find(Type);
		if (BuffMoment && RunningBuffFX.Contains(Entry.Attribute))
		{
			PlayBurst(*BuffMoment);
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoProjectileFXComponent::ApplyParams()
{
	ApplyBulletSystem();

	FProjectileParamsBase const& Params = GetOwner<AGeoProjectile>()->ResolvedParams;
	Flight.VFXComponent->SetVariableFloat(GeoNiagaraParams::BulletRadius, Params.Radius);
	Flight.VFXComponent->SetVariableLinearColor(GeoNiagaraParams::BulletHeadColor, Params.HeadColor.GetColor(1.f));
	Flight.VFXComponent->SetVariableLinearColor(GeoNiagaraParams::BulletTrailColor, Params.TrailColor.GetColor(1.f));
	Flight.VFXComponent->SetVariableFloat(GeoNiagaraParams::TrailLifetimeScale, Params.TrailLifetimeScale);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoProjectileFXComponent::StartLife()
{
	if (GeoLib::IsDedicatedServer(this))
	{
		return;
	}

	// A pooled instance can come back holding the previous shot's launch offset; the spawner re-applies its own after
	// this runs.
	Flight.VFXComponent->SetRelativeLocation(FVector::ZeroVector);
	AGeoProjectile const* const Projectile = GetOwner<AGeoProjectile>();
	StartSustainedFX(Flight, Projectile->ResolvedParams.LoopingFX);
	BindBuffFX(GeoASLib::GetGeoAscFromActor(Projectile->GetSourceOwner()));
	PlayMoment(EProjectileMoment::Start);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoProjectileFXComponent::PlayEnd(bool const bValidOverlap) const
{
	PlayMoment(EProjectileMoment::NoOverlapEnd);
	if (bValidOverlap)
	{
		PlayMoment(EProjectileMoment::ValidOverlapEnd);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoProjectileFXComponent::FadeOut(FSimpleDelegate const& OnFadedOut)
{
	StopBlinking();
	ClearBuffFX();
	Flight.AudioComponent->Stop();

	if (!GeoLib::IsDedicatedServer(this) && Flight.VFXComponent->IsActive())
	{
		PendingFadeOut = OnFadedOut;
		// Armed first: a system with nothing left to draw completes inside Deactivate.
		GetWorld()->GetTimerManager().SetTimer(
			FadeOutTimerHandle,
			FTimerDelegate::CreateUObject(this, &ThisClass::FinishFadeOut, Flight.VFXComponent.Get()),
			MaxLoopFXFadeOutDuration, false);
		Flight.VFXComponent->OnSystemFinished.AddUniqueDynamic(this, &ThisClass::FinishFadeOut);
		Flight.VFXComponent->Deactivate();
	}
	else
	{
		OnFadedOut.ExecuteIfBound();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoProjectileFXComponent::FinishFadeOut(UNiagaraComponent* /*FinishedSystem*/)
{
	GetWorld()->GetTimerManager().ClearTimer(FadeOutTimerHandle);
	Flight.VFXComponent->OnSystemFinished.RemoveDynamic(this, &ThisClass::FinishFadeOut);
	Flight.VFXComponent->DeactivateImmediate();
	PendingFadeOut.ExecuteIfBound();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoProjectileFXComponent::SetVisualLaunchLocation(FVector const& WorldLocation)
{
	Flight.VFXComponent->SetWorldLocation(WorldLocation);
	SetComponentTickEnabled(true);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoProjectileFXComponent::SetBulletRadius(float const Radius) const
{
	Flight.VFXComponent->SetVariableFloat(GeoNiagaraParams::BulletRadius, Radius);
}

// ---------------------------------------------------------------------------------------------------------------------
AActor* UGeoProjectileFXComponent::GetFXInstigator() const
{
	return GetOwner<AGeoProjectile>()->GetSourceAvatar();
}

// ---------------------------------------------------------------------------------------------------------------------
int32 UGeoProjectileFXComponent::GetAbilityLevel() const
{
	return GetOwner<AGeoProjectile>()->Payload.AbilityLevel;
}

// ---------------------------------------------------------------------------------------------------------------------
FGeoSustainedFXMoment const* UGeoProjectileFXComponent::GetBuffMoment(FGeoBuffFXEntry const& Entry) const
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
	return &Entry.ProjectileFX;
}
