// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Gem/GeoKinshipPassiveAbility.h"

#include "AbilitySystem/Abilities/Common/GeoDeployAbility.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Deployable/GeoDeployableBase.h"
#include "Characters/PlayableCharacter.h"
#include "Tool/UGeoGameplayLibrary.h"

UGeoKinshipPassiveAbility::UGeoKinshipPassiveAbility()
{
	CooldownSeconds = 5.f;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoKinshipPassiveAbility::BindEvent(UGeoAbilitySystemComponent& HolderASC)
{
	if (GeoLib::IsServer(GetWorld()))
	{
		HolderASC.OnDeployableDestroyedByDamage.AddUObject(this, &ThisClass::OnDeployableDestroyed);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoKinshipPassiveAbility::UnbindEvent(UGeoAbilitySystemComponent& HolderASC)
{
	HolderASC.OnDeployableDestroyedByDamage.RemoveAll(this);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoKinshipPassiveAbility::OnDeployableDestroyed(AGeoDeployableBase& Deployable)
{
	TArray<APlayableCharacter*> Allies = GeoLib::GetAlivePlayers(&Deployable);
	Allies.Remove(Cast<APlayableCharacter>(GetAvatarActorFromActorInfo()));
	if (!Allies.IsEmpty() && TryTrigger())
	{
		UGeoDeployAbility::SpawnDeployableAt(*GeoASLib::GetGeoAscFromActor(Allies[FMath::RandHelper(Allies.Num())]),
											 Deployable.GetActorLocation());
	}
}
