// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/GeoGameplayAbility.h"

#include "AbilitySystem/Abilities/AbilityPayload.h"
#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "Characters/PlayableCharacter.h"
#include "GameFramework/Character.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Settings/GameDataSettings.h"
#include "Tool/UGeoGameplayLibrary.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGameplayAbility::OnAvatarSet(FGameplayAbilityActorInfo const* ActorInfo, FGameplayAbilitySpec const& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);

	if (GetAssetTags().HasTag(FGeoGameplayTags::Get().Ability_Type_Passive))
	{
		ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle, false);
	}
}

// ---------------------------------------------------------------------------------------------------------------------

void UGeoGameplayAbility::ActivateAbility(FGameplayAbilitySpecHandle const Handle,
										  FGameplayAbilityActorInfo const* ActorInfo,
										  FGameplayAbilityActivationInfo const ActivationInfo,
										  FGameplayEventData const* TriggerEventData)
{
	if ((!bCommitAtActivate && !CheckCost(Handle, ActorInfo))
		|| (bCommitAtActivate && !CommitAbility(Handle, ActorInfo, ActivationInfo)))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	AActor* Instigator = GetAvatarActorFromActorInfo();
	AActor* Owner = GetOwningActorFromActorInfo();

	if (GetAssetTags().HasTag(FGeoGameplayTags::Get().Ability_Type_Passive))
	{
		StoredPayload = CreateAbilityPayload(Owner, Instigator, Instigator->GetTransform());
	}
	else
	{
		if (TriggerEventData && TriggerEventData->TargetData.Num() > 0)
		{
			if (FGeoAbilityTargetData const* TargetData =
					static_cast<FGeoAbilityTargetData const*>(TriggerEventData->TargetData.Get(0)))
			{
				StoredPayload = CreateAbilityPayload(Owner, Instigator, TargetData->Origin, TargetData->Yaw,
													 TargetData->ServerSpawnTime, TargetData->Seed);
			}
		}
		else
		{
			// Both client and server receive the same event data in a single RPC at activation
			ensureMsgf(false,
					   TEXT("No TargetData in TriggerEventData! This ability is not set as passive, it should exist"));
		}
	}

	// Bind Delegate on server to receive Fire data from client
	if (GeoLib::IsServer(GetWorld()))
	{
		UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		ASC->AbilityTargetDataSetDelegate(Handle, ActivationInfo.GetActivationPredictionKey()).RemoveAll(this);
		ASC->AbilityTargetDataSetDelegate(Handle, ActivationInfo.GetActivationPredictionKey())
			.AddUObject(this, &ThisClass::OnFireTargetDataReceived);
	}

	// Schedule fire with network delay compensation (plays montage on client, timer-only on server)
	ScheduleFireTrigger(ActivationInfo, ActorInfo->GetAnimInstance());
}

FGameplayTag UGeoGameplayAbility::GetAbilityTag() const
{
	return GeoASLib::GetAbilityTagFromAbility(*this);
}

FAbilityPayload UGeoGameplayAbility::CreateAbilityPayload(AActor* Owner, AActor* Instigator, FVector2D const& Origin,
														  float Yaw, float ServerSpawnTime, int Seed) const
{
	FAbilityPayload Payload;
	Payload.Owner = Owner;
	Payload.Instigator = Instigator;
	Payload.Origin = Origin;
	Payload.Yaw = Yaw;
	Payload.ServerSpawnTime = ServerSpawnTime;
	Payload.Seed = Seed;
	Payload.AbilityLevel = GetAbilityLevel();
	Payload.AbilityTag = GetAbilityTag();
	return Payload;
}

FAbilityPayload UGeoGameplayAbility::CreateAbilityPayload(AActor* Owner, AActor* Instigator,
														  FTransform const& Transform) const
{
	float ServerSpawnTime;
	if (GetAssetTags().HasTag(FGeoGameplayTags::Get().Ability_Type_Passive))
	{
		// Don't need server time for passive. This avoids to try get the time even if GameState does not exist still.
		ServerSpawnTime = 0.f;
	}
	else
	{
		ServerSpawnTime = GeoLib::GetServerTime(GetWorld());
	}

	return CreateAbilityPayload(Owner, Instigator, FVector2D(Transform.GetLocation()),
								Transform.GetRotation().Rotator().Yaw, ServerSpawnTime, FMath::Rand32());
}

UGeoAbilitySystemComponent* UGeoGameplayAbility::GetGeoAbilitySystemComponentFromActorInfo() const
{
	return CastChecked<UGeoAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
}

TArray<TInstancedStruct<FEffectData>> UGeoGameplayAbility::GetEffectDataArray() const
{
	TArray<TInstancedStruct<FEffectData>> FilledEffectData;
	for (auto EffectDataAsset : EffectDataAssets)
	{
		FilledEffectData.Append(GeoASLib::GetEffectDataArray(EffectDataAsset.LoadSynchronous()));
	}

	FilledEffectData.Append(EffectDataInstances);

	return FilledEffectData;
}

float UGeoGameplayAbility::GetCooldown(int32 level) const
{
	float cooldown = 0.f;

	UGameplayEffect* pCooldownEffect = GetCooldownGameplayEffect();
	if (!pCooldownEffect)
	{
		return cooldown;
	}

	pCooldownEffect->DurationMagnitude.GetStaticMagnitudeIfPossible(level, cooldown);
	return cooldown;
}

void UGeoGameplayAbility::EndAbility(FGameplayAbilitySpecHandle const Handle,
									 FGameplayAbilityActorInfo const* ActorInfo,
									 FGameplayAbilityActivationInfo const ActivationInfo, bool bReplicateEndAbility,
									 bool bWasCancelled)
{
	GetWorld()->GetTimerManager().ClearTimer(FireTriggerTimerHandle);
	FireTriggerTimerHandle.Invalidate();

	if (FireMode == EFireMode::ChargeForFireDelay)
	{
		if (APlayableCharacter* Character = Cast<APlayableCharacter>(StoredPayload.Instigator))
		{
			Character->HideDeployChargeGauge();
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGeoGameplayAbility::EndAbility(bool bReplicateEndAbility, bool bWasCancelled)
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), bReplicateEndAbility,
			   bWasCancelled);
}

void UGeoGameplayAbility::InputReleased(FGameplayAbilitySpecHandle const Handle,
										FGameplayAbilityActorInfo const* ActorInfo,
										FGameplayAbilityActivationInfo const ActivationInfo)
{
	if (FireMode == EFireMode::ChargeForFireDelay && !bIsAbilityEnding && IsActive())
	{
		GetWorld()->GetTimerManager().ClearTimer(FireTriggerTimerHandle);
		FireTriggerTimerHandle.Invalidate();
		BuildDataAndFire();
	}
}

void UGeoGameplayAbility::ScheduleFireTrigger(FGameplayAbilityActivationInfo const& ActivationInfo,
											  UAnimInstance* AnimInstance)
{
	if (FireDelay > 0.f)
	{
		if (AnimInstance && AnimMontage)
		{
			HandleAnimationMontage(AnimInstance, ActivationInfo);
		}
		GetWorld()->GetTimerManager().ClearTimer(FireTriggerTimerHandle);
		GetWorld()->GetTimerManager().SetTimer(FireTriggerTimerHandle, this, &UGeoGameplayAbility::BuildDataAndFire,
											   FireDelay);
		if (FireMode == EFireMode::ChargeForFireDelay)
		{
			ChargeStartTime = GetWorld()->GetTimeSeconds();
			if (APlayableCharacter* Character = Cast<APlayableCharacter>(StoredPayload.Instigator))
			{
				Character->ShowDeployChargeGauge(this);
			}
		}
	}
	else
	{
		BuildDataAndFire();
	}
}

void UGeoGameplayAbility::InitFireSectionIndex(UAnimInstance* AnimInstance, int32& FireSectionIndex)
{
	if (!AnimInstance->Montage_IsPlaying(AnimMontage))
	{
		FireSectionIndex = 0; // If we are not playing the montage let's reset the index.
	}
	else
	{
		++FireSectionIndex; // FireSectionIndex stored is the last played, so let's increment first
	}
}

void UGeoGameplayAbility::HandleAnimationMontage(UAnimInstance* AnimInstance,
												 FGameplayAbilityActivationInfo const& ActivationInfo)
{
	ensureMsgf(AnimMontage && AnimInstance, TEXT("No valid AnimMontage or AnimInstance"));
	UGeoAbilitySystemComponent* ASC = GetGeoAbilitySystemComponentFromActorInfo();
	int32& FireSectionIndex = ASC->GetFireSectionIndex(GetAbilityTag());
	InitFireSectionIndex(AnimInstance, FireSectionIndex);

	// Build section name: first activation goes to "Start", then cycle Fire1 -> Fire2 -> ... -> Fire1
	FName SectionToJumpTo;
	if (FireSectionIndex == 0)
	{
		SectionToJumpTo = GeoASLib::SectionStartName;
	}
	else
	{
		FString FireSectionName = GeoASLib::SectionFireString;
		FireSectionName.AppendInt(FireSectionIndex);
		if (!AnimMontage->IsValidSectionName(FName(FireSectionName)))
		{
			FireSectionIndex = 1;
			FireSectionName = GeoASLib::SectionFireString;
			FireSectionName.AppendInt(1);
		}
		SectionToJumpTo = FName(FireSectionName);
	}

	if (!AnimMontage->IsValidSectionName(SectionToJumpTo))
	{
		UE_LOG(LogGeoASC, Error, TEXT("Section %s doesn't exist ! Fallback to Start."), *SectionToJumpTo.ToString());
		SectionToJumpTo = GeoASLib::SectionStartName;
	}


	// Adjust play rate so the section fits within FireDelay
	float StartTime, EndTime;
	AnimMontage->GetSectionStartAndEndTime(AnimMontage->GetSectionIndex(SectionToJumpTo), StartTime, EndTime);
	float const SectionLength = EndTime - StartTime;
	ensureMsgf(SectionLength > 0.f, TEXT("Current section has no length"));

	float const PlayRate = SectionLength / FireDelay;

	if (!AnimInstance->Montage_IsPlaying(AnimMontage))
	{
		ASC->PlayMontage(this, ActivationInfo, AnimMontage, PlayRate, SectionToJumpTo);
	}
	else
	{
		ASC->CurrentMontageJumpToSection(SectionToJumpTo);
		AnimInstance->Montage_SetPlayRate(AnimMontage, PlayRate);
	}
}

FVector UGeoGameplayAbility::GetFireSocketLocation() const
{
	ACharacter const* Avatar = CastChecked<ACharacter>(GetAvatarActorFromActorInfo());
	USkeletalMeshComponent* Mesh = Avatar->GetMesh();
	int32 const FireSectionIndex = GetGeoAbilitySystemComponentFromActorInfo()->GetFireSectionIndex(GetAbilityTag());
	FName const SocketName{FString::Printf(TEXT("%s%d"), GeoASLib::SocketBaseName, FireSectionIndex)};

	if (!Mesh->DoesSocketExist(SocketName))
	{
		return Avatar->GetActorLocation();
	}

	return Mesh->GetSocketLocation(SocketName);
}

void UGeoGameplayAbility::SendFireDataToServer(FGeoAbilityTargetData const& AbilityTargetData) const
{
	// Client: send a fresh snapshot to the server proxy so it fires with accurate data.
	FGameplayAbilityActorInfo const* ActorInfo = GetCurrentActorInfo();
	if (ActorInfo->IsLocallyControlledPlayer())
	{
		FGameplayAbilitySpecHandle const Handle = GetCurrentAbilitySpecHandle();
		FGameplayAbilityActivationInfo const ActivationInfo = GetCurrentActivationInfo();
		UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		FScopedPredictionWindow ScopedPrediction(ASC);


		FGameplayAbilityTargetDataHandle DataHandle;
		DataHandle.Add(new FGeoAbilityTargetData(AbilityTargetData)); // Duplicate Data to ensure we keep it alive.
		ASC->ServerSetReplicatedTargetData(Handle, ActivationInfo.GetActivationPredictionKey(), DataHandle,
										   FGameplayTag{}, ASC->ScopedPredictionKey);
	}
}

FGeoAbilityTargetData UGeoGameplayAbility::BuildAbilityTargetData()
{
	AActor* const Avatar = GetAvatarActorFromActorInfo();
	ensureMsgf(IsValid(Avatar), TEXT("Avatar Actor from actor info is invalid!"));

	FVector2D const Origin = FVector2D(Avatar->GetActorLocation());
	float const Yaw = Avatar->GetActorRotation().Yaw;
	float const ServerTime = GeoLib::GetServerTime(GetWorld(), true);
	int const Seed = StoredPayload.Seed;

	return FGeoAbilityTargetData(Origin, Yaw, ServerTime, Seed);
}

void UGeoGameplayAbility::BuildDataAndFire()
{
	// This ref to AbilityTargetData needs to exist only during Fire as we create a new pointer in SendFireDataToServer
	FGeoAbilityTargetData const AbilityTargetData = BuildAbilityTargetData();
	Fire(AbilityTargetData);
}

void UGeoGameplayAbility::Fire(FGeoAbilityTargetData const& AbilityTargetData)
{
	SendFireDataToServer(AbilityTargetData);
}

void UGeoGameplayAbility::OnFireTargetDataReceived(FGameplayAbilityTargetDataHandle const& DataHandle,
												   FGameplayTag ApplicationTag)
{
	// Called on server when Fire() has happen on client and data is finally here.
	FGeoAbilityTargetData const* AbilityTargetData = static_cast<FGeoAbilityTargetData const*>(DataHandle.Get(0));
	ensureMsgf(AbilityTargetData,
			   TEXT("No FGeoAbilityTargetData found in TriggerEventData, falling back to Generate a payload"));
	StoredPayload.Seed = AbilityTargetData->Seed;
	StoredPayload.ServerSpawnTime = AbilityTargetData->ServerSpawnTime;
	StoredPayload.Origin = AbilityTargetData->Origin;
	StoredPayload.Yaw = AbilityTargetData->Yaw;
}
float UGeoGameplayAbility::GetMaxChargeTime() const
{
	return GetDefault<UGameDataSettings>()->DeployMaxChargeTime;
}

float UGeoGameplayAbility::GetChargeRatio() const
{
	if (GetMaxChargeTime() <= 0.f)
	{
		return 1.f;
	}

	float RawRatio = FMath::Clamp((GetWorld()->GetTimeSeconds() - ChargeStartTime) / GetMaxChargeTime(), 0.f, 1.f);

	UCurveFloat const* Curve = GetDefault<UGameDataSettings>()->GaugeChargingSpeedCurve.LoadSynchronous();
	if (!ensureMsgf(Curve, TEXT("GeoChargeAbility: GaugeChargingSpeedCurve is not set in GameDataSettings.")))
	{
		return RawRatio;
	}

	return FMath::Clamp(Curve->GetFloatValue(RawRatio), 0.f, 1.f);
}
// ---------------------------------------------------------------------------------------------------------------------
