// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Triangle/GeoExplosiveRecallAbility.h"

#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Turret/GeoTurretBase.h"
#include "Characters/Component/GeoDeployableManagerComponent.h"
#include "GameplayCueManager.h"
#include "Tool/UGameplayLibrary.h"

using GeoASL = UGeoAbilitySystemLibrary;

// ---------------------------------------------------------------------------------------------------------------------
void UGeoExplosiveRecallAbility::ActivateAbility(FGameplayAbilitySpecHandle const Handle,
												 FGameplayAbilityActorInfo const* ActorInfo,
												 FGameplayAbilityActivationInfo const ActivationInfo,
												 FGameplayEventData const* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (bIsAbilityEnding)
	{
		return;
	}

	AActor* Avatar = GetAvatarActorFromActorInfo();

	UGeoDeployableManagerComponent* DeployableManager = Avatar->GetComponentByClass<UGeoDeployableManagerComponent>();
	ensureMsgf(DeployableManager, TEXT("GeoExplosiveRecallAbility: No UGeoDeployableManagerComponent on avatar!"));
	if (!DeployableManager)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	struct FRecallInfo
	{
		FVector TurretLocation;
		bool bWasBlinking;
	};

	// Snapshot turret state before OnRecalled destroys the actors
	TArray<FRecallInfo> RecallInfos;
	for (AGeoDeployableBase* Deployable : TArray<TObjectPtr<AGeoDeployableBase>>(DeployableManager->GetDeployables()))
	{
		AGeoTurretBase* Turret = Cast<AGeoTurretBase>(Deployable);
		if (!IsValid(Turret))
		{
			continue;
		}
		RecallInfos.Add({Turret->GetActorLocation(), Turret->IsBlinking()});
		Turret->OnRecalled();
	}

	if (!ActorInfo->IsNetAuthority() || RecallInfos.IsEmpty())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
		return;
	}

	ensureMsgf(NormalRecallEffectData, TEXT("GeoExplosiveRecallAbility: NormalRecallEffectData is not set!"));
	ensureMsgf(BlinkBonusRecallEffectData, TEXT("GeoExplosiveRecallAbility: BlinkBonusRecallEffectData is not set!"));

	UGeoAbilitySystemComponent* PlayerASC = GetGeoAbilitySystemComponentFromActorInfo();
	FVector const AvatarLocation = Avatar->GetActorLocation();

	for (FRecallInfo const& Info : RecallInfos)
	{
		UEffectDataAsset* EffectAsset =
			Info.bWasBlinking ? BlinkBonusRecallEffectData.Get() : NormalRecallEffectData.Get();
		TArray<TInstancedStruct<FEffectData>> const EffectData = GeoASL::GetEffectDataArray(EffectAsset);

		GeoASL::ApplyEffectFromEffectData(EffectData, PlayerASC, PlayerASC, GetAbilityLevel(), StoredPayload.Seed);

		if (RecallGameplayCueTag.IsValid())
		{
			FGameplayCueParameters CueParams;
			CueParams.Location = Info.TurretLocation;
			CueParams.Normal = (AvatarLocation - Info.TurretLocation).GetSafeNormal();
			CueParams.Instigator = Avatar;
			CueParams.AbilityLevel = GetAbilityLevel();
			PlayerASC->ExecuteGameplayCue(RecallGameplayCueTag, CueParams);
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}
