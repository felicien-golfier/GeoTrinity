// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Circle/GeoMoiraBeamAbility.h"

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Deployable/HealingZone/GeoHealingZone.h"
#include "Characters/Component/GeoDeployableManagerComponent.h"
#include "Characters/Component/GeoGameFeelComponent.h"
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
	float const CurrentBeamRadius = Character->GetCapsuleComponent()->GetScaledCapsuleRadius() / 2.f
		+ HalfWidthGrowthPerAbsorbedZone * (BeamRatio - 1);

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

	float const CurrentBeamHalfWidth = Character->GetCapsuleComponent()->GetScaledCapsuleRadius() / 2.f
		+ HalfWidthGrowthPerAbsorbedZone * (BeamRatio - 1);
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

		UGeoGameFeelComponent* GameFeel = Target->FindComponentByClass<UGeoGameFeelComponent>();
		if (!IsValid(GameFeel))
		{
			ensureMsgf(GameFeel, TEXT("UGeoMoiraBeamAbility: Actor %s has no GeoGameFeelComponent"),
					   *Target->GetName());
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
				DamageEffect.bSuppressGameplayCue = !GameFeel->IsDamageCueAvailable();
				GeoASLib::ApplySingleEffectData(DamageEffect, SourceASC, TargetASC, StoredPayload.AbilityLevel,
												StoredPayload.Seed);
			}
			else if (GeoASLib::IsTeamAttitudeAligned(Character, Target, TeamAttitudeMask::FriendlyOrNeutral))
			{
				FHealEffectData HealEffect;
				HealEffect.HealAmount =
					HealPerSecond.GetValueAtLevel(StoredPayload.AbilityLevel) * BoostPerAbsorbedZone * DeltaTime;
				HealEffect.bSuppressGameplayCue = !GameFeel->IsHealCueAvailable();
				GeoASLib::ApplySingleEffectData(HealEffect, SourceASC, TargetASC, StoredPayload.AbilityLevel,
												StoredPayload.Seed);
			}
		}
	}


#ifdef WITH_EDITOR
	DrawBeamDebugLines(DeltaTime);
#endif
}
