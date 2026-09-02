// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Characters/Component/GeoGameFeelComponent.h"

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GeoTrinity/GeoTrinity.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Settings/GameDataSettings.h"
#include "Tool/UGeoGameplayLibrary.h"

UGeoGameFeelComponent::UGeoGameFeelComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UGeoGameFeelComponent::BeginPlay()
{
	Super::BeginPlay();

	TargetMesh = GetOwner()->FindComponentByClass<USkeletalMeshComponent>();
	if (!TargetMesh)
	{
		TargetMesh = GetOwner()->FindComponentByClass<UStaticMeshComponent>();
	}

	if (TargetMesh)
	{
		InitialMeshRelativeLocation = TargetMesh->GetRelativeLocation();
	}
	// A projectile draws itself with Niagara: the one owner with no mesh by design.
	else if (!GetOwner()->IsA<AGeoProjectile>())
	{
		UE_LOG(LogGeoTrinity, Warning,
			   TEXT("[GeoGameFeelComponent] No mesh found on %s — hit flash and recoil will be no-ops."),
			   *GetOwner()->GetName());
	}
}

void UGeoGameFeelComponent::TickComponent(float DeltaTime, ELevelTick TickType,
										  FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!TargetMesh)
	{
		return;
	}

	if (CurrentRecoilOffset.IsNearlyZero(0.1f))
	{
		CurrentRecoilOffset = FVector::ZeroVector;
		TargetMesh->SetRelativeLocation(InitialMeshRelativeLocation);
		SetComponentTickEnabled(false);
		return;
	}

	CurrentRecoilOffset = FMath::VInterpTo(CurrentRecoilOffset, FVector::ZeroVector, DeltaTime, RecoilRecoverySpeed);
	TargetMesh->SetRelativeLocation(InitialMeshRelativeLocation + CurrentRecoilOffset);
}

void UGeoGameFeelComponent::FlashOnHit()
{
	if (!TargetMesh || HitFlashTimerHandle.IsValid())
	{
		return;
	}

	UGameDataSettings const* GDSettings = GetDefault<UGameDataSettings>();

	bool const bIsLocalPlayer = GeoLib::IsLocalPlayerAvatar(Cast<APawn>(GetOwner()));
	TSoftObjectPtr<UMaterialInterface> const& MaterialRef =
		bIsLocalPlayer ? GDSettings->LocalPlayerHitFlashMaterial : GDSettings->HitFlashMaterial;

	UMaterialInterface* FlashMaterial = MaterialRef.LoadSynchronous();
	if (!ensureMsgf(FlashMaterial, TEXT("Fill %s in your game data settings"),
					bIsLocalPlayer ? TEXT("LocalPlayerHitFlashMaterial") : TEXT("HitFlashMaterial")))
	{
		return;
	}

	TargetMesh->SetOverlayMaterial(FlashMaterial);
	GetWorld()->GetTimerManager().SetTimer(HitFlashTimerHandle,
										   FTimerDelegate::CreateWeakLambda(this,
																			[this]()
																			{
																				TargetMesh->SetOverlayMaterial(nullptr);
																				HitFlashTimerHandle.Invalidate();
																			}),
										   GDSettings->HitFlashDuration, false);
}

void UGeoGameFeelComponent::ApplyRecoil(float Distance)
{
	// A pawn this machine doesn't control is network-smoothed through the mesh's relative location — the same slot the
	// kick writes — so recoiling it would cancel that smoothing. Turrets and deployables own their mesh outright.
	APawn const* const OwnerPawn = Cast<APawn>(GetOwner());
	if (!TargetMesh || (OwnerPawn && !OwnerPawn->IsLocallyControlled()))
	{
		return;
	}

	CurrentRecoilOffset = FVector(-1, 0, 0.f) * Distance;
	TargetMesh->SetRelativeLocation(InitialMeshRelativeLocation + CurrentRecoilOffset);
	SetComponentTickEnabled(true);
}

bool UGeoGameFeelComponent::IsCueAvailable(bool const bIsHeal)
{
	double& LastCueTime = bIsHeal ? LastHealCueTime : LastDamageCueTime;
	double const Now = GetWorld()->GetTimeSeconds();
	float const RateLimit = 1.f / GetDefault<UGameDataSettings>()->GameplayCueRateLimitPerSecond;
	if (Now - LastCueTime < RateLimit)
	{
		return false;
	}
	LastCueTime = Now;
	return true;
}

UNiagaraSystem* UGeoGameFeelComponent::GetBuffVFXSystem(FGeoBuffVFXEntry const& Entry) const
{
	AGeoProjectile const* const Projectile = Cast<AGeoProjectile>(GetOwner());
	if (!Projectile)
	{
		return UGameDataSettings::GetLoadedDataAsset(Entry.CharacterVFX);
	}

	UScriptStruct const* ScaledType = nullptr;
	if (Entry.Attribute == UCharacterAttributeSet::GetDamageMultiplierAttribute())
	{
		ScaledType = FDamageEffectData::StaticStruct();
	}
	else if (Entry.Attribute == UCharacterAttributeSet::GetAppliedHealBoostAttribute())
	{
		ScaledType = FHealEffectData::StaticStruct();
	}

	if (!ScaledType || !GeoASLib::HasEffectInArray(Projectile->EffectDataArray, ScaledType))
	{
		return nullptr;
	}
	return UGameDataSettings::GetLoadedDataAsset(Entry.ProjectileVFX);
}

void UGeoGameFeelComponent::SetBuffVFX(UNiagaraSystem* const System, bool const bShow)
{
	if (!System)
	{
		return;
	}

	int32 const Index = BuffVFXComponents.IndexOfByPredicate(
		[System](UNiagaraComponent const* const BuffVFX)
		{
			return BuffVFX->GetAsset() == System;
		});

	if (bShow == (Index != INDEX_NONE))
	{
		return;
	}

	if (!bShow)
	{
		BuffVFXComponents[Index]->DestroyComponent();
		BuffVFXComponents.RemoveAtSwap(Index);
		return;
	}

	// Niagara returns nothing when it pre-culls the spawn.
	if (UNiagaraComponent* const Spawned = UNiagaraFunctionLibrary::SpawnSystemAttached(
			System, GetOwner()->GetRootComponent(), NAME_None, FVector::ZeroVector, FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget, false))
	{
		BuffVFXComponents.Add(Spawned);
	}
}

void UGeoGameFeelComponent::BindBuffVFX(UGeoAbilitySystemComponent* const SourceASC)
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

void UGeoGameFeelComponent::RefreshBuffVFX()
{
	UGeoAbilitySystemComponent const* const SourceASC = BuffSourceASC.Get();
	if (!SourceASC)
	{
		return;
	}

	for (FGeoBuffVFXEntry const& Entry : GetDefault<UGameDataSettings>()->BuffVFX)
	{
		SetBuffVFX(GetBuffVFXSystem(Entry), GeoASLib::IsBuffed(*SourceASC, Entry.Attribute));
	}
}

void UGeoGameFeelComponent::ClearBuffVFX()
{
	if (UGeoAbilitySystemComponent* const SourceASC = BuffSourceASC.Get())
	{
		for (FGeoBuffVFXEntry const& Entry : GetDefault<UGameDataSettings>()->BuffVFX)
		{
			SourceASC->GetGameplayAttributeValueChangeDelegate(Entry.Attribute).RemoveAll(this);
		}
	}
	BuffSourceASC = nullptr;

	for (UNiagaraComponent* const BuffVFX : BuffVFXComponents)
	{
		BuffVFX->DestroyComponent();
	}
	BuffVFXComponents.Empty();
}
