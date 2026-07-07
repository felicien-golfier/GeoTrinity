// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Base/GeoChannelBeamAbility.h"

#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Characters/Component/GeoBeamVFXComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "Settings/GameDataSettings.h"
#include "Tool/UGeoGameplayLibrary.h"

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
		BeamVFXComponent->SetBeamColor(BeamColor);
		BeamVFXComponent->RegisterComponent();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoChannelBeamAbility::OnRemoveAbility(FGameplayAbilityActorInfo const* ActorInfo,
											 FGameplayAbilitySpec const& Spec)
{
	// Null when OnGiveAbility never created it (clients, or unset class). Replicated teardown removes it on clients.
	AActor const* Avatar = ActorInfo->AvatarActor.Get();
	if (UGeoBeamVFXComponent* BeamVFXComponent =
			IsValid(Avatar) ? Avatar->FindComponentByClass<UGeoBeamVFXComponent>() : nullptr)
	{
		BeamVFXComponent->DestroyComponent();
	}

	Super::OnRemoveAbility(ActorInfo, Spec);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoChannelBeamAbility::Fire(FGeoAbilityTargetData const& /*AbilityTargetData*/)
{
	bIsBeamActive = true;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoChannelBeamAbility::EndAbility(FGameplayAbilitySpecHandle const Handle,
										FGameplayAbilityActorInfo const* ActorInfo,
										FGameplayAbilityActivationInfo const ActivationInfo, bool bReplicateEndAbility,
										bool bWasCancelled)
{
	bIsBeamActive = false;

	// The component lives as long as the ability is granted (OnGive/OnRemove) — only switch the VFX off here.
	if (AActor const* Avatar = GetAvatarActorFromActorInfo())
	{
		if (UGeoBeamVFXComponent* BeamVFXComponent = Avatar->FindComponentByClass<UGeoBeamVFXComponent>())
		{
			BeamVFXComponent->SetBeamState(false, 0.f, 0.f);
		}
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ---------------------------------------------------------------------------------------------------------------------
float UGeoChannelBeamAbility::GetCurrentBeamHalfWidth(ACharacter const* Character) const
{
	return Character->GetCapsuleComponent()->GetScaledCapsuleRadius() / 2.f;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoChannelBeamAbility::Tick(float const DeltaTime)
{
	ACharacter const* const Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(Character))
	{
		return;
	}

	float const CurrentBeamHalfWidth = GetCurrentBeamHalfWidth(Character);
	// Replication only sends on change, so pushing each tick is cheap; the owning client's push is local-only
	// (lag-free) and the server's write is what replicates to everyone.
	if (UGeoBeamVFXComponent* BeamVFXComponent = Character->FindComponentByClass<UGeoBeamVFXComponent>())
	{
		BeamVFXComponent->SetBeamState(true, CurrentBeamHalfWidth * 2.f,
									   GetDefault<UGameDataSettings>()->GeneralSpellDistance);
	}
	else
	{
		// Legitimate on clients for the first frames (the component arrives via replication); a bug on the server.
		ensureMsgf(!GeoLib::IsServer(GetWorld()),
				   TEXT("UGeoChannelBeamAbility: BeamVFXComponent is missing on the server"));
	}

	TickBeam(DeltaTime,
			 GeoASLib::GetInteractableActorsInLine(
				 Character, GeoASLib::GetTeamId(Character), GetScanAttitudeMask(), true,
				 FVector2D(Character->GetActorLocation()), FVector2D(Character->GetActorForwardVector()),
				 GetDefault<UGameDataSettings>()->GeneralSpellDistance, CurrentBeamHalfWidth));
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
	FVector const BeamEnd = Origin + Forward * GetDefault<UGameDataSettings>()->GeneralSpellDistance;
	DrawDebugLine(GetWorld(), Origin + Right * CurrentBeamRadius, BeamEnd + Right * CurrentBeamRadius, FColor::Cyan,
				  false, DeltaTime);
	DrawDebugLine(GetWorld(), Origin - Right * CurrentBeamRadius, BeamEnd - Right * CurrentBeamRadius, FColor::Cyan,
				  false, DeltaTime);
	DrawDebugLine(GetWorld(), BeamEnd - Right * CurrentBeamRadius, BeamEnd + Right * CurrentBeamRadius, FColor::Cyan,
				  false, DeltaTime);
}
#endif
