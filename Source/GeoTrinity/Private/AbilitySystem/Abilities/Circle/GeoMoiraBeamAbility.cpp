// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Circle/GeoMoiraBeamAbility.h"

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Deployable/HealingZone/GeoHealingZone.h"
#include "Characters/Component/GeoBeamVFXComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "GenericTeamAgentInterface.h"
#include "Settings/GameDataSettings.h"
#include "Tool/UGeoGameplayLibrary.h"

UGeoMoiraBeamAbility::UGeoMoiraBeamAbility()
{
	CommitBehaviour = ECommitBehaviour::CostAtActivateCooldownAtEnd;
}


// ---------------------------------------------------------------------------------------------------------------------
// May run on the CDO (no primary instance yet) — derive everything from ActorInfo, never from GetWorld().
void UGeoMoiraBeamAbility::OnGiveAbility(FGameplayAbilityActorInfo const* ActorInfo, FGameplayAbilitySpec const& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	// Server only: the component replicates to clients alongside the character.

	AActor* Avatar = ActorInfo->AvatarActor.Get();
	if (!ensureMsgf(IsValid(Avatar), TEXT("UGeoMoiraBeamAbility: no avatar at OnGiveAbility — grant after InitGAS"))
		|| !GeoLib::IsServer(Avatar))
	{
		return;
	}

	if (ensureMsgf(BeamNiagaraSystem, TEXT("UGeoMoiraBeamAbility: BeamNiagaraSystem is not set — assign ")))
	{
		UGeoBeamVFXComponent* BeamVFXComponent =
			NewObject<UGeoBeamVFXComponent>(Avatar, UGeoBeamVFXComponent::StaticClass());
		BeamVFXComponent->SetNiagaraSystem(BeamNiagaraSystem);
		BeamVFXComponent->RegisterComponent();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMoiraBeamAbility::OnRemoveAbility(FGameplayAbilityActorInfo const* ActorInfo, FGameplayAbilitySpec const& Spec)
{
	// Null when OnGiveAbility never created it (clients, or unset class). Replicated teardown removes it on clients.
	AActor const* Avatar = ActorInfo->AvatarActor.Get();
	if (UGeoBeamVFXComponent* BeamVFXComponent =
			IsValid(Avatar) ? Avatar->FindComponentByClass<UGeoBeamVFXComponent>() : nullptr)
	{
		BeamVFXComponent->DestroyComponent();
	}

	Super::OnRemoveAbility(ActorInfo, Spec);
}

// ---------------------------------------------------------------------------------------------------------------------

void UGeoMoiraBeamAbility::Fire(FGeoAbilityTargetData const& AbilityTargetData)
{
	RemainingDuration = InitialDuration;
	BeamRatio = 1.f;

	if (!SpeedBuffEffect.IsValid())
	{
		ensureMsgf(false, TEXT("SpeedBuffEffect is not valid, pls fill the asset"));
		UGeoGameplayAbility::EndAbility(true, true);
		return;
	}

	if (GeoLib::IsServer(GetWorld()))
	{
		UGeoAbilitySystemComponent* SourceASC =
			Cast<UGeoAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
		SpeedBuffHandle = GeoASLib::ApplySingleEffectData(SpeedBuffEffect, SourceASC, SourceASC, GetAbilityLevel(),
														  StoredPayload.Seed);
	}
	bIsBeamActive = true;
}
// ---------------------------------------------------------------------------------------------------------------------
void UGeoMoiraBeamAbility::EndAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
									  FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
									  bool bWasCancelled)
{
	bIsBeamActive = false;
	RemainingDuration = 0.f;
	if (GeoLib::IsServer(GetWorld()))
	{
		if (SpeedBuffHandle.IsValid())
		{
			GetAbilitySystemComponentFromActorInfo()->RemoveActiveGameplayEffect(SpeedBuffHandle);
			SpeedBuffHandle.Invalidate();
		}
	}

	// The component lives as long as the ability is granted (OnGive/OnRemove) — only switch the VFX off here.
	if (AActor const* Avatar = GetAvatarActorFromActorInfo())
	{
		if (UGeoBeamVFXComponent* BeamVFXComponent = Avatar->FindComponentByClass<UGeoBeamVFXComponent>())
		{
			BeamVFXComponent->SetBeamState(false, 0.f, 0.f);
		}
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

#ifdef WITH_EDITOR
void UGeoMoiraBeamAbility::DrawBeamDebugLines(float const DeltaTime) const
{
	ACharacter const* const Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(Character))
	{
		return;
	}

	UGeoAbilitySystemComponent const* const SourceASC =
		Cast<UGeoAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	if (!SourceASC)
	{
		ensureMsgf(SourceASC, TEXT("UGeoMoiraBeamAbility: invalid ASC on activation"));
		return;
	}

	FVector const Origin = Character->GetActorLocation();
	FVector const Forward = Character->GetActorForwardVector();
	float const CurrentBeamRadius = GetCurrentBeamHalfWidth(Character);

	FVector const Right = FVector::CrossProduct(FVector::UpVector, Forward);
	FVector const BeamEnd = Origin + Forward * GetDefault<UGameDataSettings>()->GeneralSpellDistance;
	DrawDebugLine(GetWorld(), Origin + Right * CurrentBeamRadius, BeamEnd + Right * CurrentBeamRadius, FColor::Cyan,
				  false, DeltaTime);
	DrawDebugLine(GetWorld(), Origin - Right * CurrentBeamRadius, BeamEnd - Right * CurrentBeamRadius, FColor::Cyan,
				  false, DeltaTime);
	DrawDebugLine(GetWorld(), BeamEnd - Right * CurrentBeamRadius, BeamEnd + Right * CurrentBeamRadius, FColor::Cyan,
				  false, DeltaTime);
}
#endif

// ---------------------------------------------------------------------------------------------------------------------
float UGeoMoiraBeamAbility::GetCurrentBeamHalfWidth(ACharacter const* Character) const
{
	return Character->GetCapsuleComponent()->GetScaledCapsuleRadius() / 2.f
		+ HalfWidthGrowthPerAbsorbedZone * (BeamRatio - 1);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMoiraBeamAbility::Tick(float const DeltaTime)
{
	RemainingDuration -= DeltaTime;
	if (RemainingDuration <= 0.f)
	{
		UGeoGameplayAbility::EndAbility(true, false);
		return;
	}

	ACharacter const* const Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(Character))
	{
		return;
	}

	UAbilitySystemComponent* const SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SourceASC)
	{
		ensureMsgf(SourceASC, TEXT("UGeoMoiraBeamAbility: invalid ASC on activation"));
		return;
	}

	float const CurrentBeamHalfWidth = GetCurrentBeamHalfWidth(Character);
	// Replication only sends on change, so pushing each tick is cheap; the owning client's push is local-only
	// (lag-free) and the server's write is what replicates to everyone.
	if (UGeoBeamVFXComponent* BeamVFXComponent = Character->FindComponentByClass<UGeoBeamVFXComponent>())
	{
		BeamVFXComponent->SetBeamState(true, CurrentBeamHalfWidth * 2.f,
									   GetDefault<UGameDataSettings>()->GeneralSpellDistance);
	}
	else
	{
		// Legitimate on clients for the first frames (the component arrives via replication); a bug on the server.
		ensureMsgf(!GeoLib::IsServer(GetWorld()),
				   TEXT("UGeoMoiraBeamAbility: BeamVFXComponent is missing on the server"));
	}

	for (AActor* Target : GeoASLib::GetInteractableActorsInLine(
			 Character, GeoASLib::GetTeamId(Character), TeamAttitudeMask::All, false,
			 FVector2D(Character->GetActorLocation()), FVector2D(Character->GetActorForwardVector()),
			 GetDefault<UGameDataSettings>()->GeneralSpellDistance, CurrentBeamHalfWidth))
	{

		if (Target == Character)
		{
			continue;
		}

		UGeoAbilitySystemComponent* TargetASC = GeoASLib::GetGeoAscFromActor(Target);
		if (!TargetASC)
		{
			continue;
		}

		if (Target->IsA<AGeoHealingZone>()) // We get the Healing zones.
		{
			AGeoHealingZone* Zone = CastChecked<AGeoHealingZone>(Target);
			UAbilitySystemComponent* ZoneASC = Zone->GetAbilitySystemComponent();
			float const MaxHealth = ZoneASC->GetNumericAttribute(UGeoAttributeSetBase::GetMaxHealthAttribute());
			float const BeamZoneDrainPerTick = (BeamZoneDrainPercentagePerSecond / 100.f) * DeltaTime * MaxHealth;
			float const CurrentHealth = ZoneASC->GetNumericAttribute(UGeoAttributeSetBase::GetHealthAttribute());
			float const ActualDrain = FMath::Min(BeamZoneDrainPerTick, CurrentHealth);

			if (GeoLib::IsServer(GetWorld()))
			{
				FDamageEffectData DrainEffectData;
				DrainEffectData.DamageAmount = ActualDrain;
				DrainEffectData.bSuppressGameplayCue = true;
				DrainEffectData.bSuppressCombatStats = true;
				GeoASLib::ApplySingleEffectData(DrainEffectData, SourceASC, ZoneASC, GetAbilityLevel(),
												StoredPayload.Seed);
			}

			float const DrainRatio = ActualDrain / MaxHealth;
			BeamRatio += DrainRatio;
			RemainingDuration += DurationPerAbsorbedZone * DrainRatio;
		}
		else if (Target->CanBeDamaged())
		{
			float BoostPerAbsorbedZone = 1 + FMath::Max(BeamRatio - 1.f, 0.f) * DamageAndHealBoostPerAbsorbedZone;
			if (GeoASLib::IsTeamAttitudeAligned(Character, Target, TeamAttitudeMask::Hostile))
			{
				FDamageEffectData DamageEffect;
				DamageEffect.DamageAmount =
					DamagePerSecond.GetValueAtLevel(StoredPayload.AbilityLevel) * BoostPerAbsorbedZone * DeltaTime;
				DamageEffect.bLimitGameplayCue = true;
				GeoASLib::ApplySingleEffectData(DamageEffect, SourceASC, TargetASC, StoredPayload.AbilityLevel,
												StoredPayload.Seed);
			}
			else if (GeoASLib::IsTeamAttitudeAligned(Character, Target, TeamAttitudeMask::FriendlyOrNeutral))
			{
				FHealEffectData HealEffect;
				HealEffect.HealAmount =
					HealPerSecond.GetValueAtLevel(StoredPayload.AbilityLevel) * BoostPerAbsorbedZone * DeltaTime;
				HealEffect.bLimitGameplayCue = true;
				GeoASLib::ApplySingleEffectData(HealEffect, SourceASC, TargetASC, StoredPayload.AbilityLevel,
												StoredPayload.Seed);
			}
		}
	}
}
