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
#include "Tool/UGeoGameplayLibrary.h"

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
	BeamRatio = 1.f;

	if (!SpeedBuffEffect.IsValid())
	{
		ensureMsgf(SpeedBuffHandle.IsValid(), TEXT("SpeedBuffEffect is not valid, pls fill the asset"));
		return;
	}

	UGeoAbilitySystemComponent* SourceASC = Cast<UGeoAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	SpeedBuffHandle = UGeoAbilitySystemLibrary::ApplySingleEffectData(SpeedBuffEffect, SourceASC, SourceASC,
																	  GetAbilityLevel(), StoredPayload.Seed);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMoiraBeamAbility::EndAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
									  FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
									  bool bWasCancelled)
{

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

bool UGeoMoiraBeamAbility::IsInBeam(AActor const* const Actor) const
{
	ACharacter const* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	FVector const Origin = Character->GetActorLocation();
	FVector const Forward = Character->GetActorForwardVector();
	float const CurrentBeamRadius = Character->GetCapsuleComponent()->GetScaledCapsuleRadius() / 2.f
		+ RadiusGrowthPerAbsorbedZone * (BeamRatio - 1);
	FVector const ToActor = Actor->GetActorLocation() - Origin;
	float const DistAlongBeam = FMath::Clamp(FVector::DotProduct(ToActor, Forward), 0.f, BeamLength);
	FVector const ClosestPointOnAxis = Origin + Forward * DistAlongBeam;
	float const CombinedRadius = CurrentBeamRadius + Actor->GetSimpleCollisionRadius();
	return FVector::DistSquared(Actor->GetActorLocation(), ClosestPointOnAxis) <= CombinedRadius * CombinedRadius;
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
		+ RadiusGrowthPerAbsorbedZone * (BeamRatio - 1);

	FVector const Right = FVector::CrossProduct(FVector::UpVector, Forward);
	FVector const BeamEnd = Origin + Forward * BeamLength;
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

		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), false, false);

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

		if (GeoLib::IsServer(GetWorld()))
		{
			FDamageEffectData DamageEffect = FDamageEffectData();
			DamageEffect.DamageAmount = DamagePerSecond.GetValueAtLevel(BeamRatio) * DeltaTime;
			UGeoAbilitySystemLibrary::ApplySingleEffectData(DamageEffect, SourceASC, TargetASC,
															StoredPayload.AbilityLevel, StoredPayload.Seed);
		}
	}

	for (AActor* Actor :
		 UGeoAbilitySystemLibrary::GetAllAgentsWithRelationTowardsActor(Character, Character, ETeamAttitude::Friendly))
	{
		AGeoDeployableBase const* Deployable = Cast<AGeoDeployableBase>(Actor);
		if (Actor == Character || (Deployable && !Deployable->CanBeDamaged()) || !IsInBeam(Actor))
		{
			continue;
		}

		UGeoAbilitySystemComponent* TargetASC = UGeoAbilitySystemLibrary::GetGeoAscFromActor(Actor);
		if (!TargetASC)
		{
			continue;
		}

		if (GeoLib::IsServer(GetWorld()))
		{
			FHealEffectData HealEffect = FHealEffectData();
			HealEffect.HealAmount = HealPerSecond.GetValueAtLevel(BeamRatio) * DeltaTime;
			UGeoAbilitySystemLibrary::ApplySingleEffectData(HealEffect, SourceASC, TargetASC,
															StoredPayload.AbilityLevel, StoredPayload.Seed);
		}
	}

#ifdef WITH_EDITOR
	DrawBeamDebugLines(DeltaTime);
#endif

	UGeoDeployableManagerComponent* DeployableManager =
		Character->FindComponentByClass<UGeoDeployableManagerComponent>();
	if (!ensureMsgf(DeployableManager, TEXT("UGeoMoiraBeamAbility: Character has no GeoDeployableManagerComponent")))
	{
		return;
	}

	for (AGeoDeployableBase* Deployable : TArray(DeployableManager->GetDeployables()))
	{
		AGeoHealingZone* Zone = Cast<AGeoHealingZone>(Deployable);
		if (!IsValid(Zone) || !IsInBeam(Zone))
		{
			continue;
		}

		UAbilitySystemComponent* ZoneASC = Zone->GetAbilitySystemComponent();
		float const MaxHealth = ZoneASC->GetNumericAttribute(UGeoAttributeSetBase::GetMaxHealthAttribute());
		float const BeamZoneDrainPerTick = (BeamZoneDrainPercentagePerSecond / 100.f) * DeltaTime * MaxHealth;
		float const CurrentHealth = ZoneASC->GetNumericAttribute(UGeoAttributeSetBase::GetHealthAttribute());
		float const ActualDrain = FMath::Min(BeamZoneDrainPerTick, CurrentHealth);

		if (GeoLib::IsServer(GetWorld()))
		{
			FDamageEffectData DrainEffectData = FDamageEffectData();
			DrainEffectData.DamageAmount = ActualDrain;
			UGeoAbilitySystemLibrary::ApplySingleEffectData(DrainEffectData, SourceASC, ZoneASC, GetAbilityLevel(),
															StoredPayload.Seed);
		}

		float const DrainRatio = ActualDrain / MaxHealth;
		BeamRatio += DrainRatio;
		RemainingDuration += DurationPerAbsorbedZone * DrainRatio;
	}
}
