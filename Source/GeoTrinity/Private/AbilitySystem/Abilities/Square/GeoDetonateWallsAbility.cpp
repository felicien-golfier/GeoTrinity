// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Square/GeoDetonateWallsAbility.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Deployable/Wall/GeoWall.h"
#include "Settings/GameDataSettings.h"
#include "Tool/Team.h"
#include "Tool/UGeoGameplayLibrary.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoDetonateWallsAbility::Fire(FGeoAbilityTargetData const& AbilityTargetData)
{
	Super::Fire(AbilityTargetData);

	if (GeoLib::IsDedicatedServer(this))
	{
		return; // Stop here on dedicated server it will all execute from the OnFireTargetDataReceived.
	}

	FireRay(AbilityTargetData);
	EndAbility(false);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoDetonateWallsAbility::OnFireTargetDataReceived(FGameplayAbilityTargetDataHandle const& DataHandle,
														FGameplayTag const ApplicationTag)
{
	Super::OnFireTargetDataReceived(DataHandle, ApplicationTag);

	FGeoAbilityTargetData const* AbilityTargetData = static_cast<FGeoAbilityTargetData const*>(DataHandle.Get(0));
	if (!ensureMsgf(AbilityTargetData,
					TEXT("UGeoDetonateWallsAbility: no FGeoAbilityTargetData in DataHandle — cannot fire ray.")))
	{
		EndAbility(true, true);
		return;
	}

	FireRay(*AbilityTargetData);
	EndAbility(true, false);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoDetonateWallsAbility::FireRay(FGeoAbilityTargetData const& AbilityTargetData) const
{
	AActor* const Avatar = GetAvatarActorFromActorInfo();
	float const MaxRange = GetDefault<UGameDataSettings>()->GeneralSpellDistance;
	FVector2D const ForwardVector = FVector2D(FRotator(0, AbilityTargetData.Yaw, 0).Vector());

	TArray<AActor*> const ActorsInLine =
		GeoASLib::GetInteractableActorsInLine(this, GeoASLib::GetTeamId(Avatar), TeamAttitudeMask::All, false,
											  AbilityTargetData.Origin, ForwardVector, MaxRange, LineHalfWidth);

	bool const bIsServer = GeoLib::IsServer(GetWorld());
	int32 WallCount = 0;
	for (AActor* Actor : ActorsInLine)
	{
		if (AGeoWall* Wall = Cast<AGeoWall>(Actor); IsValid(Wall) && Wall->IsActive())
		{
			++WallCount;
			if (bIsServer) // Recall destroys a replicated actor — server only.
			{
				Wall->Recall();
			}
		}
	}
	float const Multiplier = WallBoostMultiplier * WallCount;
	bool bHasAppliedEffectWithABoostedValue = false;

	UGeoAbilitySystemComponent* SourceASC = GetGeoAbilitySystemComponentFromActorInfo();
	if (ensureMsgf(SourceASC, TEXT("UGeoDetonateWallsAbility: invalid ASC on server")))
	{
		for (AActor* Target : ActorsInLine)
		{
			if (Target == Avatar || Target->IsA<AGeoWall>() || !Target->CanBeDamaged())
			{
				continue;
			}

			UGeoAbilitySystemComponent* TargetASC = GeoASLib::GetGeoAscFromActor(Target);
			if (!IsValid(TargetASC))
			{
				continue;
			}

			if (GeoASLib::IsTeamAttitudeAligned(Avatar, Target, TeamAttitudeMask::Hostile))
			{
				bHasAppliedEffectWithABoostedValue = WallCount > 0;
				if (bIsServer)
				{
					FDamageEffectData DamageEffect;
					DamageEffect.DamageAmount =
						FScalableFloat(BaseDamage.GetValueAtLevel(GetAbilityLevel()) * Multiplier);
					GeoASLib::ApplySingleEffectData(DamageEffect, SourceASC, TargetASC, GetAbilityLevel(),
													AbilityTargetData.Seed, GetAbilityTag());
				}
			}
			else if (GeoASLib::IsTeamAttitudeAligned(Avatar, Target, TeamAttitudeMask::FriendlyOrNeutral))
			{
				bHasAppliedEffectWithABoostedValue = WallCount > 0;
				if (bIsServer)
				{
					FShieldEffectData ShieldEffect;
					ShieldEffect.ShieldAmount =
						FScalableFloat(BaseShield.GetValueAtLevel(GetAbilityLevel()) * Multiplier);
					GeoASLib::ApplySingleEffectData(ShieldEffect, SourceASC, TargetASC, GetAbilityLevel(),
													AbilityTargetData.Seed, GetAbilityTag());
				}
			}
		}
	}


	if (IsLocallyControlled() && FireGameplayCueTag.IsValid())
	{
		FVector2D const Endpoint = AbilityTargetData.Origin + ForwardVector * MaxRange;

		FGameplayCueParameters CueParams;
		CueParams.Location = FVector(Endpoint, ArbitraryCharacterZ);
		CueParams.Normal = FRotator(0, AbilityTargetData.Yaw, 0).Vector();
		CueParams.Instigator = Avatar;
		CueParams.AbilityLevel = GetAbilityLevel();
		CueParams.NormalizedMagnitude = bHasAppliedEffectWithABoostedValue;
		CueParams.RawMagnitude = bHasAppliedEffectWithABoostedValue;

		GeoASLib::ExecuteLocalGameplayCue(GetAbilitySystemComponentFromActorInfo(), FireGameplayCueTag, CueParams);
	}
}
