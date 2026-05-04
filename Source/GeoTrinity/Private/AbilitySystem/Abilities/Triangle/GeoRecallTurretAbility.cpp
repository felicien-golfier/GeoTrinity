// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Triangle/GeoRecallTurretAbility.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Turret/GeoTurret.h"
#include "Characters/Component/GeoDeployableManagerComponent.h"
#include "DrawDebugHelpers.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Tool/UGeoGameplayLibrary.h"

// ---------------------------------------------------------------------------------------------------------------------

void UGeoRecallTurretAbility::Fire(FGeoAbilityTargetData const& AbilityTargetData)
{
	AActor* Instigator = GetAvatarActorFromActorInfo();

	UGeoDeployableManagerComponent* DeployableManager =
		Instigator->GetComponentByClass<UGeoDeployableManagerComponent>();
	ensureMsgf(DeployableManager, TEXT("GeoRecallTurretAbility: No UGeoDeployableManagerComponent on avatar!"));
	if (!DeployableManager)
	{
		EndAbility(false, true);
		return;
	}

	TArray<FRecallInfo> RecallInfos;
	for (AGeoDeployableBase* Deployable : TArray(DeployableManager->GetDeployables()))
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
		EndAbility(false, false);
		return;
	}

	UGeoAbilitySystemComponent* PlayerASC = GetGeoAbilitySystemComponentFromActorInfo();

	for (FRecallInfo const& RecallInfo : RecallInfos)
	{
		ensureMsgf(BlinkBonusEffect.Num() > 0, TEXT("GeoRecallTurretAbility: BlinkBonusEffectData is not set!"));
		RecallInfo.Turret->Recall(true);

		if (GeoLib::IsServer(GetWorld()))
		{
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
	}

	EndAbility(false, false);
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
