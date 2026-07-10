// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Triangle/GeoRecallTurretAbility.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Deployable/Turret/GeoTurret.h"
#include "Characters/Component/GeoDeployableManagerComponent.h"
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
	for (AGeoTurret* Turret : TArray(DeployableManager->GetDeployables<AGeoTurret>()))
	{
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
		RecallInfo.Turret->Recall();

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
													StoredPayload.Seed, StoredPayload.AbilityTag);
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

	FVector2D const Origin = FVector2D(RecallInfo.TurretLocation);
	FVector2D const ToInstigator = FVector2D(Instigator->GetActorLocation()) - Origin;
	float const MaxRange = ToInstigator.Length();
	if (MaxRange <= UE_KINDA_SMALL_NUMBER)
	{
		return Targets;
	}

	for (AActor* Target : GeoASLib::GetInteractableActorsInLine(this, GeoASLib::GetTeamId(Instigator), OverlapAttitude,
																false, Origin, ToInstigator / MaxRange, MaxRange,
																LineHalfWidth))
	{
		if (Target == Instigator)
		{
			continue;
		}

		UGeoAbilitySystemComponent* TargetASC = GeoASLib::GetGeoAscFromActor(Target);
		if (IsValid(TargetASC))
		{
			Targets.Add(TargetASC);
		}
	}

	return Targets;
}
