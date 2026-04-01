// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Circle/GeoMoiraBeamAbility.h"

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "Actor/Deployable/GeoHealingZone.h"
#include "Characters/Component/GeoDeployableManagerComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "GenericTeamAgentInterface.h"
#include "Settings/GameDataSettings.h"
#include "Tool/UGameplayLibrary.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMoiraBeamAbility::ActivateAbility(FGameplayAbilitySpecHandle Handle,
										   FGameplayAbilityActorInfo const* ActorInfo,
										   FGameplayAbilityActivationInfo ActivationInfo,
										   FGameplayEventData const* TriggerEventData)
{
	// Call grandparent to commit and set up StoredPayload without scheduling fire
	UGeoGameplayAbility::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (bIsAbilityEnding)
	{
		return;
	}

	RemainingDuration = InitialDuration;
	AccumulatedRadiusBonus = 0.f;

	UGeoAbilitySystemComponent* SourceASC = Cast<UGeoAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	SpeedBuffHandle = UGeoAbilitySystemLibrary::ApplySingleEffectData(SpeedBuffEffect, SourceASC, SourceASC,
																	  GetAbilityLevel(), StoredPayload.Seed);

	GetWorld()->GetTimerManager().SetTimer(BeamTickHandle, this, &ThisClass::TickBeam,
										   GetDefault<UGameDataSettings>()->RegularTickInterval, true);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMoiraBeamAbility::EndAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
									  FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
									  bool bWasCancelled)
{
	if (UGameplayLibrary::IsServer(GetWorld()))
	{
		GetWorld()->GetTimerManager().ClearTimer(BeamTickHandle);
		if (SpeedBuffHandle.IsValid())
		{
			GetAbilitySystemComponentFromActorInfo()->RemoveActiveGameplayEffect(SpeedBuffHandle);
			SpeedBuffHandle.Invalidate();
		}
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMoiraBeamAbility::TickBeam()
{
	float const BeamTickInterval = GetDefault<UGameDataSettings>()->RegularTickInterval;
	RemainingDuration -= BeamTickInterval;
	if (RemainingDuration <= 0.f)
	{
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
		return;
	}

	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(Character))
	{
		return;
	}

	UGeoAbilitySystemComponent* SourceASC = Cast<UGeoAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	IGenericTeamAgentInterface const* OwnerTeamAgent = Cast<IGenericTeamAgentInterface>(Character);
	if (!SourceASC || !OwnerTeamAgent)
	{
		return;
	}

	FVector const Origin = Character->GetActorLocation();
	FVector const Forward = Character->GetActorForwardVector();
	float CurrentBeamRadius = Character->GetCapsuleComponent()->GetScaledCapsuleRadius() / 2.f + AccumulatedRadiusBonus;

	FVector const Right = FVector::CrossProduct(FVector::UpVector, Forward);
	FVector const BeamEnd = Origin + Forward * BeamLength;
	DrawDebugLine(GetWorld(), Origin + Right * CurrentBeamRadius, BeamEnd + Right * CurrentBeamRadius, FColor::Cyan,
				  false, BeamTickInterval);
	DrawDebugLine(GetWorld(), Origin - Right * CurrentBeamRadius, BeamEnd - Right * CurrentBeamRadius, FColor::Cyan,
				  false, BeamTickInterval);
	DrawDebugLine(GetWorld(), BeamEnd - Right * CurrentBeamRadius, BeamEnd + Right * CurrentBeamRadius, FColor::Cyan,
				  false, BeamTickInterval);

	auto IsInBeam = [&](AActor const* Actor) -> bool
	{
		FVector const ToActor = Actor->GetActorLocation() - Origin;
		float const DistAlongBeam = FMath::Clamp(FVector::DotProduct(ToActor, Forward), 0.f, BeamLength);
		FVector const ClosestPointOnAxis = Origin + Forward * DistAlongBeam;
		float const CombinedRadius = CurrentBeamRadius + Actor->GetSimpleCollisionRadius();
		return FVector::DistSquared(Actor->GetActorLocation(), ClosestPointOnAxis) <= CombinedRadius * CombinedRadius;
	};

	for (AActor* Actor :
		 UGeoAbilitySystemLibrary::GetAllAgentsWithRelationTowardsActor(Character, Character, ETeamAttitude::Hostile))
	{
		if (!IsInBeam(Actor))
		{
			continue;
		}
		UGeoAbilitySystemComponent* TargetASC = UGeoAbilitySystemLibrary::GetGeoAscFromActor(Actor);
		if (!TargetASC)
		{
			continue;
		}

		UGeoAbilitySystemLibrary::ApplySingleEffectData(DamageEffect, SourceASC, TargetASC, GetAbilityLevel(),
														StoredPayload.Seed);
	}

	for (AActor* Actor :
		 UGeoAbilitySystemLibrary::GetAllAgentsWithRelationTowardsActor(Character, Character, ETeamAttitude::Friendly))
	{
		if (Actor == Character || !IsInBeam(Actor))
		{
			continue;
		}
		UGeoAbilitySystemComponent* TargetASC = UGeoAbilitySystemLibrary::GetGeoAscFromActor(Actor);
		if (!TargetASC)
		{
			continue;
		}
		UGeoAbilitySystemLibrary::ApplySingleEffectData(HealEffect, SourceASC, TargetASC, GetAbilityLevel(),
														StoredPayload.Seed);
	}

	UGeoDeployableManagerComponent* DeployableManager =
		Character->FindComponentByClass<UGeoDeployableManagerComponent>();
	if (!DeployableManager)
	{
		return;
	}

	TArray<TObjectPtr<AGeoDeployableBase>> const ZonesCopy = DeployableManager->GetDeployables();
	for (AGeoDeployableBase* Deployable : ZonesCopy)
	{
		AGeoHealingZone* Zone = Cast<AGeoHealingZone>(Deployable);
		if (!IsValid(Zone) || !IsInBeam(Zone))
		{
			continue;
		}

		UAbilitySystemComponent* ZoneASC = Zone->GetAbilitySystemComponent();
		float const MaxHealth = ZoneASC->GetNumericAttribute(UGeoAttributeSetBase::GetMaxHealthAttribute());
		float const BeamZoneDrainPerTick = (BeamZoneDrainPercentagePerSecond / 100.f) * BeamTickInterval * MaxHealth;
		float const CurrentHealth = ZoneASC->GetNumericAttribute(UGeoAttributeSetBase::GetHealthAttribute());
		float const ActualDrain = FMath::Min(BeamZoneDrainPerTick, CurrentHealth);

		FDamageEffectData DrainEffectData = FDamageEffectData();
		DrainEffectData.DamageAmount = ActualDrain;
		UGeoAbilitySystemLibrary::ApplySingleEffectData(DrainEffectData, SourceASC, ZoneASC, GetAbilityLevel(),
														StoredPayload.Seed);

		float const DrainRatio = ActualDrain / MaxHealth;
		RemainingDuration += DurationPerAbsorbedZone * DrainRatio;
		float const RadiusBonus = RadiusGrowthPerZone * DrainRatio;
		AccumulatedRadiusBonus += RadiusBonus;
	}
}
