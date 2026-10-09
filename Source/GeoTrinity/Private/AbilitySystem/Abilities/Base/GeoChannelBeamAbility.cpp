// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Base/GeoChannelBeamAbility.h"

#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Characters/Component/GeoBeamVFXComponent.h"
#include "Characters/Component/GeoIndicatorComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "Tool/UGeoGameplayLibrary.h"

// ---------------------------------------------------------------------------------------------------------------------
UGeoChannelBeamAbility::UGeoChannelBeamAbility()
	// Only game-thread constructions may register as tickable: async-loaded Blueprint CDOs are built on the loading
	// thread and must not register (registration is game-thread-only; the CDO never ticks anyway).
	: FTickableGameObject(IsInGameThread() ? ETickableTickType::Conditional : ETickableTickType::Never)
{
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoChannelBeamAbility::ActivateAbility(FGameplayAbilitySpecHandle const Handle,
											 FGameplayAbilityActorInfo const* ActorInfo,
											 FGameplayAbilityActivationInfo const ActivationInfo,
											 FGameplayEventData const* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// Telegraphs where the beam will land during the fire-delay windup, at the same dimensions Fire() starts with.
	ACharacter const* const Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UGeoIndicatorComponent* const IndicatorComponent =
		IsValid(Character) ? Character->FindComponentByClass<UGeoIndicatorComponent>() : nullptr;
	// No fire delay: Super already fired, nothing is left to telegraph.
	if (IndicatorComponent && GetFireDelay() > 0.f)
	{
		FGeoIndicatorState State;
		State.Shape = EGeoIndicatorShape::Ray;
		State.bAttachToOwner = true;
		State.Size = {GeoASLib::GetSpellDistance(GetAbilitySystemComponentFromActorInfo()),
					  GetCurrentBeamHalfWidth(Character) * 2.f};
		State.Colors = GeoColor::GetMeaningColors(BeamColor, SecondaryBeamColors);
		State.StartServerTime = StoredPayload.ServerSpawnTime;
		State.Duration = GetFireDelay();
		WindupIndicatorHandle = IndicatorComponent->AddIndicator(MoveTemp(State));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoChannelBeamAbility::RemoveWindupIndicator()
{
	AActor const* const Avatar = GetAvatarActorFromActorInfo();
	if (UGeoIndicatorComponent* const IndicatorComponent =
			IsValid(Avatar) ? Avatar->FindComponentByClass<UGeoIndicatorComponent>() : nullptr)
	{
		IndicatorComponent->RemoveIndicator(WindupIndicatorHandle);
	}

	WindupIndicatorHandle = INDEX_NONE;
}

// ---------------------------------------------------------------------------------------------------------------------
// May run on the CDO (no primary instance yet) — derive everything from ActorInfo, never from GetWorld().
void UGeoChannelBeamAbility::OnGiveAbility(FGameplayAbilityActorInfo const* ActorInfo, FGameplayAbilitySpec const& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	// Server only: the component replicates to clients alongside the character.

	AActor* Avatar = ActorInfo->AvatarActor.Get();
	if (!ensureMsgf(IsValid(Avatar), TEXT("UGeoChannelBeamAbility: no avatar at OnGiveAbility — grant after InitGAS"))
		|| !GeoLib::IsServer(Avatar))
	{
		return;
	}

	if (ensureMsgf(BeamNiagaraSystem, TEXT("UGeoChannelBeamAbility: BeamNiagaraSystem is not set — assign ")))
	{
		UGeoBeamVFXComponent* BeamVFXComponent =
			NewObject<UGeoBeamVFXComponent>(Avatar, UGeoBeamVFXComponent::StaticClass());
		BeamVFXComponent->SetNiagaraSystem(BeamNiagaraSystem);
		BeamVFXComponent->SetBeamColors(GeoColor::GetMeaningColors(BeamColor, SecondaryBeamColors));
		BeamVFXComponent->RegisterComponent();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoChannelBeamAbility::OnRemoveAbility(FGameplayAbilityActorInfo const* ActorInfo,
											 FGameplayAbilitySpec const& Spec)
{
	// Server only: replicated teardown removes it on clients. Destroying here on the client races the class-change
	// replication — the next class's component can arrive before this removal fires, and FindComponentByClass would
	// tear that one down instead, leaving the client without a beam.
	AActor const* Avatar = ActorInfo->AvatarActor.Get();
	if (UGeoBeamVFXComponent* BeamVFXComponent = IsValid(Avatar) && GeoLib::IsServer(Avatar)
			? Avatar->FindComponentByClass<UGeoBeamVFXComponent>()
			: nullptr)
	{
		BeamVFXComponent->DestroyComponent();
	}

	Super::OnRemoveAbility(ActorInfo, Spec);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoChannelBeamAbility::Fire(FGeoAbilityTargetData const& /*AbilityTargetData*/)
{
	RemoveWindupIndicator();
	bIsBeamActive = true;
	// Every machine starts the beam now; only the server keeps it updated, its replication can land much later.
	PushBeamState(Cast<ACharacter>(GetAvatarActorFromActorInfo()));
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoChannelBeamAbility::PushBeamState(ACharacter const* const Character) const
{
	UGeoBeamVFXComponent* BeamVFXComponent =
		IsValid(Character) ? Character->FindComponentByClass<UGeoBeamVFXComponent>() : nullptr;
	if (ensureMsgf(BeamVFXComponent, TEXT("UGeoChannelBeamAbility: BeamVFXComponent is missing on the avatar")))
	{
		BeamVFXComponent->SetBeamState(true, GetCurrentBeamHalfWidth(Character) * 2.f,
									   GeoASLib::GetSpellDistance(GetAbilitySystemComponentFromActorInfo()),
									   GetBeamDuration());
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoChannelBeamAbility::EndAbility(FGameplayAbilitySpecHandle const Handle,
										FGameplayAbilityActorInfo const* ActorInfo,
										FGameplayAbilityActivationInfo const ActivationInfo, bool bReplicateEndAbility,
										bool bWasCancelled)
{
	bIsBeamActive = false;
	RemoveWindupIndicator();

	// The component lives as long as the ability is granted (OnGive/OnRemove) — only switch the VFX off here.
	if (AActor const* Avatar = GetAvatarActorFromActorInfo())
	{
		if (UGeoBeamVFXComponent* BeamVFXComponent = Avatar->FindComponentByClass<UGeoBeamVFXComponent>())
		{
			BeamVFXComponent->SetBeamState(false, 0.f, 0.f);
		}
	}

	if (UAnimInstance* AnimInstance = GetActorInfo().GetAnimInstance(); AnimInstance && AnimMontage)
	{
		AnimInstance->Montage_Stop(AnimMontage->GetDefaultBlendOutTime(), AnimMontage);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ---------------------------------------------------------------------------------------------------------------------
float UGeoChannelBeamAbility::GetCurrentBeamHalfWidth(ACharacter const* Character) const
{
	return Character->GetCapsuleComponent()->GetScaledCapsuleRadius() / 2.f;
}

// ---------------------------------------------------------------------------------------------------------------------
float UGeoChannelBeamAbility::GetBeamDuration() const
{
	// By default, the ChannelBeam ability doesn't have duration. Needs to be overriden
	return 0.f;
}
void UGeoChannelBeamAbility::Tick(float const DeltaTime)
{
	ACharacter const* const Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(Character))
	{
		return;
	}

	float const CurrentBeamHalfWidth = GetCurrentBeamHalfWidth(Character);
	// Server only: its write replicates to everyone, the client's own simulation of the width can drift from it.
	// Replication only sends on change, so pushing each tick is cheap.
	if (GeoLib::IsServer(GetWorld()))
	{
		PushBeamState(Character);
	}

	TickBeam(DeltaTime,
			 GeoASLib::GetInteractableActorsInLine(
				 Character, GeoASLib::GetTeamId(Character), GetScanAttitudeMask(), false,
				 FVector2D(Character->GetActorLocation()), FVector2D(Character->GetActorForwardVector()),
				 GeoASLib::GetSpellDistance(GetAbilitySystemComponentFromActorInfo()), CurrentBeamHalfWidth));
}

#ifdef WITH_EDITOR
// ---------------------------------------------------------------------------------------------------------------------
void UGeoChannelBeamAbility::DrawBeamDebugLines(float const DeltaTime) const
{
	ACharacter const* const Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(Character))
	{
		return;
	}

	FVector const Origin = Character->GetActorLocation();
	FVector const Forward = Character->GetActorForwardVector();
	float const CurrentBeamRadius = GetCurrentBeamHalfWidth(Character);

	FVector const Right = FVector::CrossProduct(FVector::UpVector, Forward);
	FVector const BeamEnd = Origin + Forward * GeoASLib::GetSpellDistance(GetAbilitySystemComponentFromActorInfo());
	DrawDebugLine(GetWorld(), Origin + Right * CurrentBeamRadius, BeamEnd + Right * CurrentBeamRadius, FColor::Cyan,
				  false, DeltaTime);
	DrawDebugLine(GetWorld(), Origin - Right * CurrentBeamRadius, BeamEnd - Right * CurrentBeamRadius, FColor::Cyan,
				  false, DeltaTime);
	DrawDebugLine(GetWorld(), BeamEnd - Right * CurrentBeamRadius, BeamEnd + Right * CurrentBeamRadius, FColor::Cyan,
				  false, DeltaTime);
}
#endif
