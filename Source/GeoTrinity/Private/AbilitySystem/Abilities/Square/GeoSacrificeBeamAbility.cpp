// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Square/GeoSacrificeBeamAbility.h"

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "AbilitySystem/Types/GeoAscTypes.h"
#include "Actor/Deployable/Wall/GeoWall.h"
#include "Characters/Component/GeoDeployableManagerComponent.h"
#include "Characters/GeoCharacter.h"
#include "Tool/UGeoGameplayLibrary.h"

UGeoSacrificeBeamAbility::UGeoSacrificeBeamAbility()
{
	// Shares its input with the detonate ability: a held button must never chain-activate across the pair.
	bActivateOnFreshPressOnly = true;
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoSacrificeBeamAbility::TryRedirectIncomingDamage(UAbilitySystemComponent& VictimASC,
														 FGameplayEffectContextHandle const& DamageContext,
														 float const Damage)
{
	FGeoGameplayEffectContext const* GeoDamageContext =
		static_cast<FGeoGameplayEffectContext const*>(DamageContext.Get());
	if ((GeoDamageContext && GeoDamageContext->DoNotRedirectSacrifice())
		|| !VictimASC.HasMatchingGameplayTag(FGeoGameplayTags::Get().Status_Sacrificed))
	{
		return false;
	}

	// The mark GE's context identifies the Square that applied it — the mark itself is the registry.
	TArray<FActiveGameplayEffectHandle> const MarkHandles =
		VictimASC.GetActiveEffects(FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(
			FGameplayTagContainer(FGeoGameplayTags::Get().Status_Sacrificed)));
	FActiveGameplayEffect const* MarkEffect =
		MarkHandles.Num() > 0 ? VictimASC.GetActiveGameplayEffect(MarkHandles[0]) : nullptr;
	UAbilitySystemComponent* SquareASC =
		MarkEffect ? MarkEffect->Spec.GetContext().GetOriginalInstigatorAbilitySystemComponent() : nullptr;
	if (!ensureMsgf(IsValid(SquareASC),
					TEXT("UGeoSacrificeBeamAbility: victim %s has Status.Sacrificed but no mark GE with a valid "
						 "instigator ASC"),
					*VictimASC.GetOwnerActor()->GetName()))
	{
		return false;
	}

	AGeoCharacter const* SquareCharacter = Cast<AGeoCharacter>(SquareASC->GetAvatarActor());
	if (!IsValid(SquareCharacter) || SquareCharacter->IsDead())
	{
		return false;
	}

	// GetPrimaryInstance is null for InstancedPerExecution (project default) — search the live instances instead.
	UGeoSacrificeBeamAbility* Instance = nullptr;
	for (FGameplayAbilitySpec const& Spec : SquareASC->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->IsA<UGeoSacrificeBeamAbility>())
		{
			for (UGameplayAbility* AbilityInstance : Spec.GetAbilityInstances())
			{
				if (IsValid(AbilityInstance) && AbilityInstance->IsActive())
				{
					Instance = Cast<UGeoSacrificeBeamAbility>(AbilityInstance);
					break;
				}
			}
			break;
		}
	}
	if (!ensureMsgf(Instance,
					TEXT("UGeoSacrificeBeamAbility: sacrifice mark present but no active ability instance on the "
						 "instigator — marks should be removed in EndAbility")))
	{
		return false;
	}

	Instance->RedirectCapturedDamage(Damage);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSacrificeBeamAbility::Fire(FGeoAbilityTargetData const& AbilityTargetData)
{
	if (!ensureMsgf(SacrificeMarkEffect.IsValid(),
					TEXT("UGeoSacrificeBeamAbility: SacrificeMarkEffect is not set — assign GE_SacrificeMark"))
		|| !ensureMsgf(DetonateReadyEffect.IsValid(),
					   TEXT("UGeoSacrificeBeamAbility: DetonateReadyEffect is not set — assign GE_DetonateReady")))
	{
		UGeoGameplayAbility::EndAbility(true, true);
		return;
	}

	ChannelElapsed = 0.f;
	if (GeoLib::IsServer(GetWorld()))
	{
		UGeoAbilitySystemComponent* SourceASC = GetGeoAbilitySystemComponentFromActorInfo();
		GeoASLib::ApplySingleEffectData(DetonateReadyEffect, SourceASC, SourceASC, GetAbilityLevel(),
										StoredPayload.Seed, StoredPayload.AbilityTag);
	}
	Super::Fire(AbilityTargetData);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSacrificeBeamAbility::EndAbility(FGameplayAbilitySpecHandle const Handle,
										  FGameplayAbilityActorInfo const* ActorInfo,
										  FGameplayAbilityActivationInfo const ActivationInfo,
										  bool bReplicateEndAbility, bool bWasCancelled)
{
	if (GeoLib::IsServer(GetWorld()))
	{
		RemoveAllSacrificeMarks();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSacrificeBeamAbility::Tick(float const DeltaTime)
{
	ChannelElapsed += DeltaTime;
	if (ChannelElapsed >= MaxChannelDuration)
	{
		UGeoGameplayAbility::EndAbility(true, false);
		return;
	}

	Super::Tick(DeltaTime);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSacrificeBeamAbility::TickBeam(float const /*DeltaTime*/, TArray<AActor*> const& ActorsInLine)
{
	if (!GeoLib::IsServer(GetWorld()))
	{
		return; // Marking and redirection are authoritative; clients see the marks via GE cue replication.
	}

	UGeoAbilitySystemComponent* SourceASC = GetGeoAbilitySystemComponentFromActorInfo();
	TSet<AActor const*> CurrentTargets;
	for (AActor* Target : ActorsInLine)
	{
		UGeoAbilitySystemComponent* TargetASC = GeoASLib::GetGeoAscFromActor(Target);
		// The Square is the final redirect receiver — never sacrifice it. Walls CAN be sacrificed; the redirect
		// shares they receive (and their own drain) carry bDoNotRedirectSacrifice so nothing loops.
		if (Target == StoredPayload.Instigator || !IsValid(TargetASC))
		{
			continue;
		}

		CurrentTargets.Add(Target);
		if (!SacrificedActors.Contains(Target))
		{
			SacrificedActors.Add(Target,
								 GeoASLib::ApplySingleEffectData(SacrificeMarkEffect, SourceASC, TargetASC,
																 GetAbilityLevel(), StoredPayload.Seed,
																 StoredPayload.AbilityTag));
		}
	}

	for (auto It = SacrificedActors.CreateIterator(); It; ++It)
	{
		if (It->Key.IsValid() && CurrentTargets.Contains(It->Key.Get()))
		{
			continue;
		}
		if (UGeoAbilitySystemComponent* VictimASC =
				It->Key.IsValid() ? GeoASLib::GetGeoAscFromActor(It->Key.Get()) : nullptr;
			IsValid(VictimASC))
		{
			VictimASC->RemoveActiveGameplayEffect(It->Value);
		}
		It.RemoveCurrent();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSacrificeBeamAbility::RedirectCapturedDamage(float const Damage)
{
	UGeoAbilitySystemComponent* SourceASC = GetGeoAbilitySystemComponentFromActorInfo();
	SourceASC->SetNumericAttributeBase(
		UCharacterAttributeSet::GetSacrificeValueAttribute(),
		SourceASC->GetNumericAttribute(UCharacterAttributeSet::GetSacrificeValueAttribute()) + Damage);

	TArray<AGeoWall*> AliveWalls;
	if (UGeoDeployableManagerComponent* DeployableManager = IsValid(StoredPayload.Instigator)
			? StoredPayload.Instigator->FindComponentByClass<UGeoDeployableManagerComponent>()
			: nullptr)
	{
		for (AGeoWall* Wall : DeployableManager->GetDeployables<AGeoWall>())
		{
			if (IsValid(Wall) && Wall->IsActive() && Wall->CanBeDamaged() && Wall->GetOwner() == StoredPayload.Owner)
			{
				AliveWalls.Add(Wall);
			}
		}
	}

	FDamageEffectData RedirectEffect;
	RedirectEffect.DamageAmount = FScalableFloat(Damage / (AliveWalls.Num() + 1));
	RedirectEffect.bDoNotRedirectSacrifice = true;
	RedirectEffect.bLimitGameplayCue = true;

	// Each apply can kill its receiver and cascade (the Square's death force-expires every wall), so the snapshot
	// goes stale mid-loop: re-validate each wall at apply time, and damage the Square last.
	for (AGeoWall* Wall : AliveWalls)
	{
		UGeoAbilitySystemComponent* WallASC = IsValid(Wall) ? GeoASLib::GetGeoAscFromActor(Wall) : nullptr;
		if (IsValid(WallASC))
		{
			GeoASLib::ApplySingleEffectData(RedirectEffect, SourceASC, WallASC, GetAbilityLevel(), StoredPayload.Seed,
											StoredPayload.AbilityTag);
		}
	}
	GeoASLib::ApplySingleEffectData(RedirectEffect, SourceASC, SourceASC, GetAbilityLevel(), StoredPayload.Seed,
									StoredPayload.AbilityTag);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSacrificeBeamAbility::RemoveAllSacrificeMarks()
{
	for (auto const& [Victim, MarkHandle] : SacrificedActors)
	{
		if (UGeoAbilitySystemComponent* VictimASC =
				Victim.IsValid() ? GeoASLib::GetGeoAscFromActor(Victim.Get()) : nullptr;
			IsValid(VictimASC))
		{
			VictimASC->RemoveActiveGameplayEffect(MarkHandle);
		}
	}
	SacrificedActors.Empty();
}
