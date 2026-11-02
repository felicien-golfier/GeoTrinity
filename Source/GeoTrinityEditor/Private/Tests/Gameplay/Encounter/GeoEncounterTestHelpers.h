// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Encounter/Action/GeoActivateAbilityAction.h"
#include "Encounter/GeoEncounterAction.h"
#include "Encounter/GeoEncounterSubsystem.h"
#include "Engine/GameInstance.h"
#include "GameClasses/Match/GeoGameState.h"
#include "Misc/AutomationTest.h"
#include "Tests/Gameplay/Fake/GeoTestEncounter.h"

using FEncounterActions = TArray<TObjectPtr<UGeoEncounterAction>>;

/** A game instance and a game state in World, so an enemy can spawn. */
inline void PrepareEnemyWorld(FAutomationTestBase& Test, UWorld& World)
{
	World.SetGameInstance(NewObject<UGameInstance>(GEngine));
	World.SetGameState(World.SpawnActor<AGeoGameState>());
}

/** A caster arena in World, its hidden caster spawned. The adds it spawns are test enemies with no DefaultAttributes.
 */
inline AGeoTestCasterArena* SpawnCasterArena(FAutomationTestBase& Test, UWorld& World)
{
	PrepareEnemyWorld(Test, World);
	Test.AddExpectedError(TEXT("Missing DefaultAttributes"), EAutomationExpectedErrorFlags::Contains, 0);
	return World.SpawnActor<AGeoTestCasterArena>();
}

/** The ASC of World's shared caster, AbilityClass granted to it; Handle is its spec. */
inline UGeoAbilitySystemComponent* GrantShared(UWorld& World, TSubclassOf<UGeoGameplayAbility> const AbilityClass,
											   FGameplayAbilitySpecHandle& Handle)
{
	UGeoEncounterSubsystem* const Subsystem = World.GetSubsystem<UGeoEncounterSubsystem>();
	Handle = Subsystem->GrantSharedAbility(AbilityClass);
	return GeoASLib::GetGeoAscFromActor(Subsystem->GetSharedCaster());
}

inline UGeoActivateAbilityAction* NewActivateAction(AGeoTestEncounter& Encounter,
													TSubclassOf<UGeoGameplayAbility> const AbilityClass,
													bool const bWaitForEnd = true)
{
	UGeoActivateAbilityAction* const Action = NewObject<UGeoActivateAbilityAction>(&Encounter);
	Action->AbilityClass = AbilityClass;
	Action->bWaitForEnd = bWaitForEnd;
	return Action;
}
