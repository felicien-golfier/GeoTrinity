// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Triangle/GeoRecallTurretAbility.h"

#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Turret/GeoTurret.h"
#include "Characters/Component/GeoDeployableManagerComponent.h"
#include "DrawDebugHelpers.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Tool/UGeoGameplayLibrary.h"

using GeoASLib = UGeoAbilitySystemLibrary;

// ---------------------------------------------------------------------------------------------------------------------
void UGeoRecallTurretAbility::ActivateAbility(FGameplayAbilitySpecHandle const Handle,
											  FGameplayAbilityActorInfo const* ActorInfo,
											  FGameplayAbilityActivationInfo const ActivationInfo,
											  FGameplayEventData const* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (bIsAbilityEnding)
	{
		return;
	}

	AActor* Instigator = GetAvatarActorFromActorInfo();

	UGeoDeployableManagerComponent* DeployableManager =
		Instigator->GetComponentByClass<UGeoDeployableManagerComponent>();
	ensureMsgf(DeployableManager, TEXT("GeoRecallTurretAbility: No UGeoDeployableManagerComponent on avatar!"));
	if (!DeployableManager)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	TArray<FRecallInfo> RecallInfos;
	for (AGeoDeployableBase* Deployable : TArray<TObjectPtr<AGeoDeployableBase>>(DeployableManager->GetDeployables()))
	{
		AGeoTurret* Turret = Cast<AGeoTurret>(Deployable);
		if (!IsValid(Turret))
		{
			continue;
		}
		RecallInfos.Add({Turret, Turret->GetActorLocation(), Turret->IsBlinking()});
	}

	if (RecallInfos.IsEmpty())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
		return;
	}

	UGeoAbilitySystemComponent* PlayerASC = GetGeoAbilitySystemComponentFromActorInfo();
	FVector const AvatarLocation = Instigator->GetActorLocation();

	for (FRecallInfo const& RecallInfo : RecallInfos)
	{
		if (GeoLib::IsServer(GetWorld()))
		{
			ensureMsgf(BlinkBonusEffect.Num() > 0, TEXT("GeoRecallTurretAbility: BlinkBonusEffectData is not set!"));
			RecallInfo.Turret->OnRecalled();

			TArray<TInstancedStruct<FEffectData>> EffectData = GetEffectDataArray();
			if (RecallInfo.bWasBlinking)
			{
				EffectData.Append(BlinkBonusEffect);
			}

			for (auto const TargetASC : FindTargets(Instigator, RecallInfo))
			{
				GeoASLib::ApplyEffectFromEffectData(EffectData, PlayerASC, TargetASC, GetAbilityLevel(),
													StoredPayload.Seed);
			}
		}

		FireRecallCue(PlayerASC, RecallInfo, AvatarLocation);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoRecallTurretAbility::FireRecallCue(UGeoAbilitySystemComponent* PlayerASC, FRecallInfo const& RecallInfo,
											FVector const& AvatarLocation) const
{
	if (!RecallGameplayCueTag.IsValid())
	{
		DrawDebugLine(GetWorld(), RecallInfo.TurretLocation, AvatarLocation,
					  RecallInfo.bWasBlinking ? FColor::Red : FColor::Cyan, false, 3.0f, 0, 2.0f);
		return;
	}

	FGameplayCueParameters CueParams;
	CueParams.Location = RecallInfo.TurretLocation;
	CueParams.Normal = (AvatarLocation - RecallInfo.TurretLocation).GetSafeNormal();
	CueParams.Instigator = GetAvatarActorFromActorInfo();
	CueParams.AbilityLevel = GetAbilityLevel();
	CueParams.RawMagnitude = RecallInfo.bWasBlinking ? 1.f : 0.f;
	PlayerASC->ExecuteGameplayCue(RecallGameplayCueTag, CueParams);
}

// ---------------------------------------------------------------------------------------------------------------------
TArray<UGeoAbilitySystemComponent*> UGeoRecallTurretAbility::FindTargets(AActor const* Instigator,
																		 FRecallInfo const& RecallInfo) const
{
	TArray<UGeoAbilitySystemComponent*> Targets{};
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Instigator);

	FCollisionObjectQueryParams ObjectQueryParams = FCollisionObjectQueryParams(ECC_GeoCharacter);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);

	TArray<FHitResult> HitResults;
	bool const bHit = GetWorld()->LineTraceMultiByObjectType(
		HitResults, RecallInfo.TurretLocation, Instigator->GetActorLocation(), ObjectQueryParams, QueryParams);

	if (bHit)
	{
		for (FHitResult const& Hit : HitResults)
		{
			if (AActor* HitActor = Hit.GetActor();
				IsValid(HitActor) && GeoASLib::IsTeamAttitudeAligned(Instigator, HitActor, OverlapAttitude))
			{
				UGeoAbilitySystemComponent* HitActorASC = HitActor->GetComponentByClass<UGeoAbilitySystemComponent>();
				if (IsValid(HitActorASC))
				{
					Targets.Add(HitActorASC);
				}
			}
		}
	}

	return Targets;
}
