// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Triangle/GeoRecallTurretAbility.h"

#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Turret/GeoTurret.h"
#include "Characters/Component/GeoDeployableManagerComponent.h"
#include "DrawDebugHelpers.h"
#include "GameplayCueManager.h"
#include "GeoTrinity/GeoTrinity.h"
#include "UObject/ICookInfo.h"

using GeoASL = UGeoAbilitySystemLibrary;

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
	ensureMsgf(DeployableManager, TEXT("GeoExplosiveRecallAbility: No UGeoDeployableManagerComponent on avatar!"));
	if (!DeployableManager)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	// Snapshot turret state before OnRecalled destroys the actors
	TArray<FRecallInfo> RecallInfos;
	for (AGeoDeployableBase* Deployable : TArray<TObjectPtr<AGeoDeployableBase>>(DeployableManager->GetDeployables()))
	{
		AGeoTurret* Turret = Cast<AGeoTurret>(Deployable);
		if (!IsValid(Turret))
		{
			continue;
		}
		RecallInfos.Add({Turret->GetActorLocation(), Turret->IsBlinking()});
		Turret->OnRecalled();
	}

	if (RecallInfos.IsEmpty())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
		return;
	}

	ensureMsgf(NormalRecallEffectData, TEXT("GeoExplosiveRecallAbility: NormalRecallEffectData is not set!"));
	ensureMsgf(BlinkBonusRecallEffectData, TEXT("GeoExplosiveRecallAbility: BlinkBonusRecallEffectData is not set!"));

	UGeoAbilitySystemComponent* PlayerASC = GetGeoAbilitySystemComponentFromActorInfo();
	FVector const AvatarLocation = Instigator->GetActorLocation();

	for (FRecallInfo const& RecallInfo : RecallInfos)
	{
		DrawDebugLine(GetWorld(), RecallInfo.TurretLocation, AvatarLocation, FColor::Cyan, false, 3.0f, 0, 2.0f);

		UEffectDataAsset* EffectAsset =
			RecallInfo.bWasBlinking ? BlinkBonusRecallEffectData.Get() : NormalRecallEffectData.Get();
		TArray<TInstancedStruct<FEffectData>> const EffectData = GeoASL::GetEffectDataArray(EffectAsset);

		for (auto const TargetASC : FindTargets(Instigator, RecallInfo))
		{
			GeoASL::ApplyEffectFromEffectData(EffectData, PlayerASC, TargetASC, GetAbilityLevel(), StoredPayload.Seed);
		}

		if (RecallGameplayCueTag.IsValid())
		{
			FGameplayCueParameters CueParams;
			CueParams.Location = RecallInfo.TurretLocation;
			CueParams.Normal = (AvatarLocation - RecallInfo.TurretLocation).GetSafeNormal();
			CueParams.Instigator = Instigator;
			CueParams.AbilityLevel = GetAbilityLevel();
			PlayerASC->ExecuteGameplayCue(RecallGameplayCueTag, CueParams);
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}

TArray<UGeoAbilitySystemComponent*> UGeoRecallTurretAbility::FindTargets(AActor const* Instigator,
																		 FRecallInfo const& RecallInfo) const
{
	TArray<UGeoAbilitySystemComponent*> Targets{};
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Instigator);

	FCollisionObjectQueryParams ObjectQueryParams = FCollisionObjectQueryParams(ECC_GeoCharacter); // GeoCharacter ECC
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
				IsValid(HitActor) && UGameplayLibrary::IsTeamAttitudeAligned(Instigator, HitActor, OverlapAttitude))
			{
				UGeoAbilitySystemComponent* HitActorASC = HitActor->GetComponentByClass<UGeoAbilitySystemComponent>();
				if (IsValid(HitActorASC))
				{
					DrawDebugCircle(GetWorld(), HitActor->GetActorLocation(), 50.0f, 32, FColor::Green, false, 3.0f, 0,
									2.0f, FVector::ForwardVector, FVector::RightVector);
					Targets.Add(HitActorASC);
				}
			}
		}
	}

	return Targets;
}
