// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"

#include "AbilitySystem/Abilities/Base/AbilityPayload.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "Characters/PlayableCharacter.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Settings/GameDataSettings.h"
#include "Tool/UGeoGameplayLibrary.h"

void UGeoGameplayAbility::ActivateAbility(FGameplayAbilitySpecHandle const Handle,
										  FGameplayAbilityActorInfo const* ActorInfo,
										  FGameplayAbilityActivationInfo const ActivationInfo,
										  FGameplayEventData const* TriggerEventData)
{
	if ((CommitBehaviour == ECommitBehaviour::DoNotAutoCommit && !CheckCost(Handle, ActorInfo))
		|| (CommitBehaviour == ECommitBehaviour::CostAtActivateCooldownAtEnd
			&& !CommitAbilityCost(Handle, ActorInfo, ActivationInfo))
		|| (CommitBehaviour == ECommitBehaviour::AtActivate && !CommitAbility(Handle, ActorInfo, ActivationInfo)))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	bool const bIsPassive = IsPassive();
	bool const bHasTargetData = TriggerEventData && TriggerEventData->TargetData.Num() > 0;
	// Server-driven abilities build their payload from avatar state: no client ever sends them target data. That covers
	// ServerOnly too, not just ServerInitiated — an AI ability fired by STTask_FireAbility carries no TriggerEventData.
	bool const bIsServerDriven = GetNetExecutionPolicy() == EGameplayAbilityNetExecutionPolicy::ServerInitiated
		|| GetNetExecutionPolicy() == EGameplayAbilityNetExecutionPolicy::ServerOnly;
	ensureMsgf(bIsServerDriven || bIsPassive || bHasTargetData,
			   TEXT("No TargetData in TriggerEventData! This ability is not set as passive, it should exist"));
	FGeoAbilityTargetData const* TargetData =
		bHasTargetData ? static_cast<FGeoAbilityTargetData const*>(TriggerEventData->TargetData.Get(0)) : nullptr;
	ensureMsgf(bIsServerDriven || bIsPassive || TargetData, TEXT("Target Data 0 is not a FGeoAbilityTargetData"));

	if (!bIsPassive && bHasTargetData && TargetData)
	{
		StoredPayload = CreateAbilityPayloadFromTargetData(*TargetData);
	}
	else
	{
		StoredPayload = CreateAbilityPayload();
	}

	// Bind Delegate on server to receive Fire data from client
	if (GeoLib::IsServer(GetWorld()))
	{
		ClampRemoteClientOrigin();

		UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		ASC->AbilityTargetDataSetDelegate(Handle, ActivationInfo.GetActivationPredictionKey()).RemoveAll(this);
		ASC->AbilityTargetDataSetDelegate(Handle, ActivationInfo.GetActivationPredictionKey())
			.AddUObject(this, &ThisClass::OnFireTargetDataReceived);

		if (RemoteFireTag.IsValid())
		{
			ASC->AddLooseGameplayTag(RemoteFireTag, 1, EGameplayTagReplicationState::TagOnly);
			bRemoteFireTagApplied = true;
		}
	}

	// Schedule fire with network delay compensation (plays montage on client, timer-only on server)
	ScheduleFireTrigger(ActivationInfo, ActorInfo->GetAnimInstance());
}

bool UGeoGameplayAbility::CanActivateAbility(FGameplayAbilitySpecHandle const Handle,
											 FGameplayAbilityActorInfo const* ActorInfo,
											 FGameplayTagContainer const* SourceTags,
											 FGameplayTagContainer const* TargetTags,
											 FGameplayTagContainer* OptionalRelevantTags) const
{
	AGeoCharacter const* Character = ActorInfo ? Cast<AGeoCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (Character && Character->IsDead())
	{
		return false;
	}

	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

FGameplayTag UGeoGameplayAbility::GetAbilityTag() const
{
	return GeoASLib::GetAbilityTagFromAbility(*this);
}

FAbilityPayload UGeoGameplayAbility::CreateAbilityPayloadFromTargetData(FGeoAbilityTargetData const& TargetData) const
{
	return CreateAbilityPayload(TargetData.Origin, TargetData.Yaw, TargetData.ServerSpawnTime, TargetData.Seed);
}

FAbilityPayload UGeoGameplayAbility::CreateAbilityPayload() const
{
	int const Seed = GetNewSeed();
	return CreateAbilityPayload(
		GetFireOrigin2D(GetAvatarActorFromActorInfo(), GetGeoAbilitySystemComponentFromActorInfo(), Seed),
		GetFireYaw(GetAvatarActorFromActorInfo(), Seed), GetStartTime(GetWorld()), Seed);
}

FAbilityPayload UGeoGameplayAbility::CreateAbilityPayload(FVector2D const& Origin, float const Yaw,
														  float const ServerSpawnTime, int const Seed) const
{
	FAbilityPayload Payload;
	Payload.Owner = GetOwningActorFromActorInfo();
	Payload.Instigator = GetAvatarActorFromActorInfo();
	Payload.Origin = Origin;
	Payload.Yaw = Yaw;
	Payload.ServerSpawnTime = ServerSpawnTime;
	Payload.Seed = Seed;
	Payload.AbilityLevel = GetAbilityLevel();
	Payload.AbilityTag = GetAbilityTag();
	Payload.OpenHitNotification();
	return Payload;
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
		UEffectDataAsset const* const LoadedAsset = EffectDataAsset.LoadSynchronous();
		if (ensureMsgf(IsValid(LoadedAsset), TEXT("%hs: %s lists an EffectDataAsset that failed to load"), __FUNCTION__,
					   *GetName()))
		{
			FilledEffectData.Append(LoadedAsset->EffectDataInstances);
		}
	}

	FilledEffectData.Append(EffectDataInstances);

	return FilledEffectData;
}

float UGeoGameplayAbility::GetCooldown(int32 AbilityLevel) const
{
	float Cooldown = 0.f;

	UGameplayEffect* CooldownEffect = GetCooldownGameplayEffect();
	if (!CooldownEffect)
	{
		return Cooldown;
	}

	CooldownEffect->DurationMagnitude.GetStaticMagnitudeIfPossible(AbilityLevel, Cooldown);
	return Cooldown;
}

void UGeoGameplayAbility::EndAbility(FGameplayAbilitySpecHandle const Handle,
									 FGameplayAbilityActorInfo const* ActorInfo,
									 FGameplayAbilityActivationInfo const ActivationInfo, bool bReplicateEndAbility,
									 bool bWasCancelled)
{
	if (CommitBehaviour == ECommitBehaviour::CostAtActivateCooldownAtEnd && !bWasCancelled)
	{
		CommitAbilityCooldown(Handle, ActorInfo, ActivationInfo, true);
	}

	GetWorld()->GetTimerManager().ClearTimer(FireTriggerTimerHandle);
	FireTriggerTimerHandle.Invalidate();

	if (bRemoteFireTagApplied)
	{
		bRemoteFireTagApplied = false;
		GetAbilitySystemComponentFromActorInfo()->RemoveLooseGameplayTag(RemoteFireTag, 1,
																		 EGameplayTagReplicationState::TagOnly);
	}

	if (FireMode == EFireMode::ChargeForFireDelay)
	{
		if (APlayableCharacter* Character = Cast<APlayableCharacter>(StoredPayload.Instigator))
		{
			SetChargeGaugeVisible(Character, false);
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
	if (FireMode == EFireMode::ChargeForFireDelay && !bIsAbilityEnding && IsActive()
		&& FireTriggerTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(FireTriggerTimerHandle);
		FireTriggerTimerHandle.Invalidate();
		BuildDataAndFire();
		UAnimInstance* AnimInstance = GetActorInfo().GetAnimInstance();
		if (AnimInstance && AnimMontage && IsLocallyControlled())
		{
			GetAbilitySystemComponentFromActorInfo()->CurrentMontageJumpToSection(GeoASLib::SectionEndName);
		}
	}
}

float UGeoGameplayAbility::GetFireDelay() const
{
	return bUseGeneralChargeTimeForFireDelay ? GetDefault<UGameDataSettings>()->GeneralChargeTime : FireDelay;
}

void UGeoGameplayAbility::ScheduleFireTrigger(FGameplayAbilityActivationInfo const& ActivationInfo,
											  UAnimInstance* AnimInstance)
{
	if (GetFireDelay() > 0.f)
	{
		HandleAnimationMontage(AnimInstance, ActivationInfo);
		GetWorld()->GetTimerManager().ClearTimer(FireTriggerTimerHandle);
		GetWorld()->GetTimerManager().SetTimer(FireTriggerTimerHandle, this, &UGeoGameplayAbility::BuildDataAndFire,
											   GetFireDelay());
		if (FireMode == EFireMode::ChargeForFireDelay)
		{
			ChargeStartTime = GetWorld()->GetTimeSeconds();
			if (APlayableCharacter* Character = Cast<APlayableCharacter>(StoredPayload.Instigator))
			{
				SetChargeGaugeVisible(Character, true);
			}
		}
	}
	else
	{
		BuildDataAndFire();
	}
}

int32& UGeoGameplayAbility::GetFireSectionIndex(UGeoAbilitySystemComponent* ASC, UAnimInstance const* AnimInstance)
{
	int32& FireSectionIndex = ASC->GetFireSectionIndex(GetAbilityTag());
	bool const bHasAnyFireSection = AnimMontage->IsValidSectionName(GeoASLib::SectionFireName)
		|| AnimMontage->IsValidSectionName(FName(*FString::Printf(TEXT("%s1"), *GeoASLib::SectionFireString)));
	bool const bIsMontagePlaying = AnimInstance && AnimInstance->Montage_IsPlaying(AnimMontage);
	if (bHasAnyFireSection && bIsMontagePlaying)
	{
		++FireSectionIndex;
	}
	else
	{
		FireSectionIndex = 0;
	}

	return FireSectionIndex;
}

/**
 * Selects and plays the correct montage section for the current shot:
 *  - FireSectionIndex == 0 → "Start" section (first activation of the ability)
 *  - FireSectionIndex >= 1 → "Fire1", "Fire2", ... cycling back to "Fire1" when out of sections
 * Adjusts play rate so the chosen section exactly fills the FireDelay window, keeping animation
 * in sync regardless of how long the designer makes each section.
 */
void UGeoGameplayAbility::HandleAnimationMontage(UAnimInstance* AnimInstance,
												 FGameplayAbilityActivationInfo const& ActivationInfo)
{
	if (!AnimMontage || !AnimInstance)
	{
		return;
	}

	UGeoAbilitySystemComponent* ASC = GetGeoAbilitySystemComponentFromActorInfo();
	int32& FireSectionIndex = GetFireSectionIndex(ASC, AnimInstance);

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

	float const PlayRate = SectionLength / GetFireDelay();

	// Only PlayMontage bumps the replicated PlayInstanceId, the single thing that makes a non-owning client restart the
	// montage. A section jump reaches it as a position correction, which does nothing once that client's montage
	// instance ended — and it ends whenever the correction lands a ping too late. So every shot plays outright.
	ASC->PlayMontage(this, ActivationInfo, AnimMontage, PlayRate, SectionToJumpTo);
}

void UGeoGameplayAbility::SendFireDataToServer(FGeoAbilityTargetData const& AbilityTargetData) const
{
	// Client: send a fresh snapshot to the server proxy so it fires with accurate data.
	FGameplayAbilityActorInfo const* ActorInfo = GetCurrentActorInfo();
	if (ActorInfo->IsLocallyControlledPlayer() && !GeoLib::IsServer(this))
	{
		FGameplayAbilitySpecHandle const Handle = GetCurrentAbilitySpecHandle();
		FGameplayAbilityActivationInfo const ActivationInfo = GetCurrentActivationInfo();
		UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

		FGameplayAbilityTargetDataHandle DataHandle;
		DataHandle.Add(new FGeoAbilityTargetData(AbilityTargetData)); // Duplicate Data to ensure we keep it alive.
		ASC->ServerSetReplicatedTargetData(Handle, ActivationInfo.GetActivationPredictionKey(), DataHandle,
										   FGameplayTag{}, ASC->ScopedPredictionKey);
	}
}

void UGeoGameplayAbility::UpdatePayloadFromTargetData(FGeoAbilityTargetData const& TargetData, bool const bKeepSeed)
{
	if (!bKeepSeed)
	{
		StoredPayload.Seed = TargetData.Seed;
	}
	StoredPayload.ServerSpawnTime = TargetData.ServerSpawnTime;
	StoredPayload.Origin = TargetData.Origin;
	StoredPayload.Yaw = TargetData.Yaw;
	StoredPayload.OpenHitNotification();
}

void UGeoGameplayAbility::ClampRemoteClientOrigin()
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (IsLocallyControlled() || !IsValid(Avatar))
	{
		return;
	}

	float const MaxDeviation = GetDefault<UGameDataSettings>()->MaxFireOriginDeviation;
	if (FVector2D::DistSquared(StoredPayload.Origin, FVector2D(Avatar->GetActorLocation()))
		<= FMath::Square(MaxDeviation))
	{
		return;
	}

	UE_LOG(LogGeoASC, Warning, TEXT("%s: rejected fire origin %s from %s, snapped to fire socket."),
		   *GetAbilityTag().ToString(), *StoredPayload.Origin.ToString(), *Avatar->GetName());
	StoredPayload.Origin = GetFireOrigin2D(Avatar, GetGeoAbilitySystemComponentFromActorInfo(), StoredPayload.Seed);
}

FGeoAbilityTargetData UGeoGameplayAbility::GetUpdatedTargetData()
{
	FVector2D const Origin =
		GetFireOrigin2D(StoredPayload.Instigator, GetGeoAbilitySystemComponentFromActorInfo(), StoredPayload.Seed);
	float const Yaw = GetFireYaw(StoredPayload.Instigator, StoredPayload.Seed);
	float const ServerTime = GetStartTime(GetWorld());
	return FGeoAbilityTargetData(Origin, Yaw, ServerTime, StoredPayload.Seed);
}

void UGeoGameplayAbility::BuildDataAndFire()
{
	// This ref to AbilityTargetData needs to exist only during Fire as we create a new pointer in SendFireDataToServer
	FGeoAbilityTargetData const AbilityTargetData = GetUpdatedTargetData();
	UpdatePayloadFromTargetData(AbilityTargetData);

	// The activation prediction window closed when ActivateAbility returned, so the shot needs one of its own: without
	// it a client fails HasAuthorityOrPredictionKey and silently applies neither cost nor Cooldown, leaving it free to
	// re-press until the server's Cooldown lands a ping later and rejects the activation. SendFireDataToServer hands
	// this key to the server, which re-enters it around OnFireTargetDataReceived, so both sides commit under it.
	FScopedPredictionWindow ScopedPrediction(GetAbilitySystemComponentFromActorInfo());
	Fire(AbilityTargetData);
}

void UGeoGameplayAbility::Fire(FGeoAbilityTargetData const& AbilityTargetData)
{
	if (!GeoLib::IsServer(this))
	{
		SendFireDataToServer(AbilityTargetData);
	}
}

void UGeoGameplayAbility::OnFireTargetDataReceived(FGameplayAbilityTargetDataHandle const& DataHandle,
												   FGameplayTag ApplicationTag)
{
	// Called on server when Fire() has happen on client and data is finally here.
	FGeoAbilityTargetData const* AbilityTargetData = static_cast<FGeoAbilityTargetData const*>(DataHandle.Get(0));
	if (!ensureMsgf(AbilityTargetData,
					TEXT("No FGeoAbilityTargetData found in DataHandle — cannot update StoredPayload.")))
	{
		return;
	}

	UpdatePayloadFromTargetData(*AbilityTargetData);
}

/**
 * Returns the world-space spawn point for ability.
 * Socket names follow the convention "<SocketBaseName><FireSectionIndex>" (e.g. "anim_socket_0", "anim_socket_1").
 * Falls back to the actor origin when no matching socket exists (e.g. enemy shapes without rigs).
 */
FVector2D UGeoGameplayAbility::GetFireOrigin2D(AActor* Instigator, UGeoAbilitySystemComponent* SourceASC, int) const
{
	ACharacter const* Character = Cast<ACharacter>(Instigator);
	if (IsValid(Character) && Character->GetMesh())
	{
		USkeletalMeshComponent const* Mesh = Character->GetMesh();
		int32 const FireSectionIndex = SourceASC->GetFireSectionIndex(GetAbilityTag());
		FName const SocketName{FString::Printf(TEXT("%s%d"), GeoASLib::SocketBaseName, FireSectionIndex)};

		if (Mesh->DoesSocketExist(SocketName))
		{
			return FVector2D(Mesh->GetSocketLocation(SocketName));
		}
	}

	return IsValid(Instigator) ? FVector2D(Instigator->GetActorLocation()) : FVector2D::ZeroVector;
}

FVector UGeoGameplayAbility::GetFireOrigin(AActor* Instigator, UGeoAbilitySystemComponent* SourceASC,
										   int const Seed) const
{
	return FVector(GetFireOrigin2D(Instigator, SourceASC, Seed), ArbitraryCharacterZ);
}

void UGeoGameplayAbility::RemoteFireShot(AActor* /*Avatar*/, UGeoAbilitySystemComponent* /*SourceASC*/) const
{
	ensureMsgf(false, TEXT("%s carries a RemoteFireTag but does not override RemoteFireShot — other clients see nothing."),
			   *GetName());
}

int32 UGeoGameplayAbility::GetPlayingFireSectionIndex(AActor const* Avatar) const
{
	ACharacter const* Character = Cast<ACharacter>(Avatar);
	USkeletalMeshComponent const* Mesh = IsValid(Character) ? Character->GetMesh() : nullptr;
	UAnimInstance const* AnimInstance = Mesh ? Mesh->GetAnimInstance() : nullptr;
	if (!AnimInstance || !AnimMontage)
	{
		return 0;
	}

	FString const SectionName = AnimInstance->Montage_GetCurrentSection(AnimMontage).ToString();
	if (!SectionName.StartsWith(GeoASLib::SectionFireString))
	{
		return 0;
	}

	return FCString::Atoi(*SectionName.RightChop(GeoASLib::SectionFireString.Len()));
}

void UGeoGameplayAbility::SpawnRemoteProjectiles(AActor* Avatar, UGeoAbilitySystemComponent* SourceASC,
												 FExternalProjectileParams const& Params,
												 EProjectileTarget const Target) const
{
	int const Seed = GetNewSeed();
	FVector const Origin = GetFireOrigin(Avatar, SourceASC, Seed);
	float const Yaw = GetFireYaw(Avatar, Seed);

	FAbilityPayload Payload;
	Payload.Owner = SourceASC->GetOwnerActor();
	Payload.Instigator = Avatar;
	Payload.Origin = FVector2D(Origin);
	Payload.Yaw = Yaw;
	Payload.Seed = Seed;
	Payload.AbilityLevel = SourceASC->GetCombatLevel();
	Payload.AbilityTag = GetAbilityTag();

	GeoASLib::SpawnProjectileSpread(Avatar->GetWorld(), Params, Target, Origin, Yaw, 0.f, Payload, {});
}

void UGeoGameplayAbility::SetChargeGaugeVisible(APlayableCharacter* Character, bool bVisible)
{
	// Gauge is local UI: show only on the machine controlling this pawn.
	if (GeoLib::IsLocalPlayerAvatar(Character))
	{
		Character->SetDeployChargeGaugeVisibility(this, bVisible);
	}
}

bool UGeoGameplayAbility::IsPassive() const
{
	return GetAssetTags().HasTag(FGeoGameplayTags::Get().Ability_Type_Passive);
}

int UGeoGameplayAbility::GetNewSeed() const
{
	return FMath::Rand32();
}

float UGeoGameplayAbility::GetStartTime(UWorld const* World) const
{
	if (IsPassive())
	{
		// Don't need server time for passive. This avoids to try get the time even if GameState does not exist still.
		return 0.f;
	}

	return GeoLib::GetServerTime(World, true);
}

float UGeoGameplayAbility::GetFireYaw(AActor const* Instigator, int const Seed) const
{
	return IsValid(Instigator) ? Instigator->GetActorRotation().Yaw : 0.f;
}

// ---------------------------------------------------------------------------------------------------------------------

float UGeoGameplayAbility::GetChargeRatio() const
{
	float const MaxChargeTime = GetFireDelay();
	if (MaxChargeTime <= 0.f || FireMode != EFireMode::ChargeForFireDelay)
	{
		return 1.f;
	}

	float RawRatio = FMath::Clamp((GetWorld()->GetTimeSeconds() - ChargeStartTime) / MaxChargeTime, 0.f, 1.f);

	return ApplyChargingCurve(RawRatio);
}

// ---------------------------------------------------------------------------------------------------------------------

float UGeoGameplayAbility::ApplyChargingCurve(float RawRatio) const
{
	// Apply a designer-tunable easing curve so the gauge feels responsive at the start and slows near full charge.
	UCurveFloat const* Curve = GetDefault<UGameDataSettings>()->GaugeChargingSpeedCurve.LoadSynchronous();
	if (!ensureMsgf(Curve, TEXT("GeoChargeAbility: GaugeChargingSpeedCurve is not set in GameDataSettings.")))
	{
		return RawRatio;
	}

	return FMath::Clamp(Curve->GetFloatValue(RawRatio), 0.f, 1.f);
}
