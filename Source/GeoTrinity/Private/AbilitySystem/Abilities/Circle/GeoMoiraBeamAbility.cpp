// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Circle/GeoMoiraBeamAbility.h"

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Deployable/HealingZone/GeoHealingZone.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GenericTeamAgentInterface.h"
#include "Tool/UGeoGameplayLibrary.h"

UGeoMoiraBeamAbility::UGeoMoiraBeamAbility()
{
	CommitBehaviour = ECommitBehaviour::CostAtActivateCooldownAtEnd;
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
														  StoredPayload.Seed, StoredPayload.AbilityTag);
	}
	Super::Fire(AbilityTargetData);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMoiraBeamAbility::EndAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
									  FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
									  bool bWasCancelled)
{
	RemainingDuration = 0.f;
	if (GeoLib::IsServer(GetWorld()))
	{
		if (SpeedBuffHandle.IsValid())
		{
			GetAbilitySystemComponentFromActorInfo()->RemoveActiveGameplayEffect(SpeedBuffHandle);
			SpeedBuffHandle.Invalidate();
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ---------------------------------------------------------------------------------------------------------------------
float UGeoMoiraBeamAbility::GetCurrentBeamHalfWidth(ACharacter const* Character) const
{
	return Super::GetCurrentBeamHalfWidth(Character) + HalfWidthGrowthPerAbsorbedZone * (BeamRatio - 1);
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

	Super::Tick(DeltaTime);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMoiraBeamAbility::TickBeam(float const DeltaTime, TArray<AActor*> const& ActorsInLine)
{
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

	for (AActor* Target : ActorsInLine)
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

			float BoostFinishedZone = 0.f;
			if (ActualDrain >= CurrentHealth && !FinishedZones.Contains(Zone))
			{
				FinishedZones.Add(Zone);
				BoostFinishedZone = FinishedZones.Num() * BoostPerFinishedZone;
			}

			if (GeoLib::IsServer(GetWorld()))
			{
				FDamageEffectData DrainEffectData;
				DrainEffectData.DamageAmount = ActualDrain;
				DrainEffectData.bSuppressGameplayCue = true;
				DrainEffectData.bSuppressCombatStats = true;
				DrainEffectData.bDoNotRedirectSacrifice = true;
				GeoASLib::ApplySingleEffectData(DrainEffectData, SourceASC, ZoneASC, GetAbilityLevel(),
												StoredPayload.Seed, StoredPayload.AbilityTag);
			}

			float const DrainRatio = ActualDrain / MaxHealth;
			BeamRatio = FMath::Min(BeamRatio + DrainRatio + BoostFinishedZone, MaximumZoneAbsorbed + 1.f);
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
												StoredPayload.Seed, StoredPayload.AbilityTag);
			}
			else if (GeoASLib::IsTeamAttitudeAligned(Character, Target, TeamAttitudeMask::FriendlyOrNeutral))
			{
				FHealEffectData HealEffect;
				HealEffect.HealAmount =
					HealPerSecond.GetValueAtLevel(StoredPayload.AbilityLevel) * BoostPerAbsorbedZone * DeltaTime;
				HealEffect.bLimitGameplayCue = true;
				GeoASLib::ApplySingleEffectData(HealEffect, SourceASC, TargetASC, StoredPayload.AbilityLevel,
												StoredPayload.Seed, StoredPayload.AbilityTag);
			}
		}
	}
}
