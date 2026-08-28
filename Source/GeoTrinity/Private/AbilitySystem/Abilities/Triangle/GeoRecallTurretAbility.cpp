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

	TArray<FRecallInfo> RecallInfos;
	for (AGeoTurret* Turret : GetActiveTurrets(Instigator))
	{
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

			// Each turret's drag-back is its own shot: one hit reported per turret
			StoredPayload.OpenHitNotification();
			for (auto const TargetASC : FindTargets(Instigator, RecallInfo))
			{
				GeoASLib::ApplyEffectFromEffectData(EffectData, PlayerASC, TargetASC, GetAbilityLevel(),
													StoredPayload.Seed, StoredPayload.AbilityTag);
				GeoASLib::NotifyAbilityHit(StoredPayload, TargetASC->GetAvatarActor());
			}
		}
	}

	EndAbility(false, false);
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoRecallTurretAbility::CanActivateAbility(FGameplayAbilitySpecHandle const Handle,
												 FGameplayAbilityActorInfo const* ActorInfo,
												 FGameplayTagContainer const* SourceTags,
												 FGameplayTagContainer const* TargetTags,
												 FGameplayTagContainer* OptionalRelevantTags) const
{
	return ActorInfo && !GetActiveTurrets(ActorInfo->AvatarActor.Get()).IsEmpty()
		&& Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

// ---------------------------------------------------------------------------------------------------------------------
TArray<AGeoTurret*> UGeoRecallTurretAbility::GetActiveTurrets(AActor const* Avatar) const
{
	TArray<AGeoTurret*> Turrets;
	if (!IsValid(Avatar))
	{
		return Turrets;
	}

	UGeoDeployableManagerComponent const* DeployableManager =
		Avatar->GetComponentByClass<UGeoDeployableManagerComponent>();
	if (!ensureMsgf(DeployableManager, TEXT("GeoRecallTurretAbility: No UGeoDeployableManagerComponent on avatar!")))
	{
		return Turrets;
	}

	for (AGeoTurret* Turret : DeployableManager->GetDeployables<AGeoTurret>())
	{
		if (IsValid(Turret) && (Turret->IsActive() || Turret->IsBlinking()))
		{
			Turrets.Add(Turret);
		}
	}
	return Turrets;
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

	for (AActor* Target :
		 GeoASLib::GetInteractableActorsInLine(this, GeoASLib::GetTeamId(Instigator), OverlapAttitude, false, Origin,
											   ToInstigator / MaxRange, MaxRange, LineHalfWidth))
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
