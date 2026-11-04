// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/Hazard/LateralWavePattern.h"
#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "Actor/Buttons/GeoTeleporter.h"
#include "Actor/Deployable/Zones/GeoEffectZone.h"
#include "Actor/GeoCat.h"
#include "Actor/GeoTargetPoint.h"
#include "Characters/EnemyCharacter.h"
#include "Characters/GeoEnemySetup.h"
#include "Characters/PlayableCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Encounter/Action/Cat/GeoCatMontageAction.h"
#include "Encounter/Action/Cat/GeoCatRoamAction.h"
#include "Encounter/Action/Cat/GeoCatWalkAction.h"
#include "Encounter/Action/GeoActivateAbilityAction.h"
#include "Encounter/Action/GeoAddToCounterAction.h"
#include "Encounter/Action/GeoCatLineAction.h"
#include "Encounter/Action/GeoCatLineFromFileAction.h"
#include "Encounter/Action/GeoDelayAction.h"
#include "Encounter/Action/GeoQuestCardAction.h"
#include "Encounter/Action/GeoSetActorHiddenAction.h"
#include "Encounter/Action/GeoSetMovementLockedAction.h"
#include "Encounter/Action/GeoSetTeleporterOpenAction.h"
#include "Encounter/Action/GeoShowAbilityPageAction.h"
#include "Encounter/Action/GeoSpawnEnemiesAction.h"
#include "Encounter/GeoEncounterSubsystem.h"
#include "Encounter/Trigger/GeoAbilityActivatedTrigger.h"
#include "Encounter/Trigger/GeoActorDiedTrigger.h"
#include "Encounter/Trigger/GeoCounterTrigger.h"
#include "Encounter/Trigger/GeoEventTrigger.h"
#include "Encounter/Trigger/GeoHealthThresholdTrigger.h"
#include "Encounter/Trigger/GeoPlayerIdleTrigger.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "GameClasses/Match/GeoGameState.h"
#include "GameFramework/DefaultPawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "GameplayTagsManager.h"
#include "HAL/FileManager.h"
#include "Materials/Material.h"
#include "Misc/AutomationTest.h"
#include "Misc/DataValidation.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Settings/GameDataSettings.h"
#include "StateTree.h"
#include "Tests/Gameplay/Fake/GeoTestDeployable.h"
#include "Tests/Gameplay/Fake/GeoTestEncounter.h"
#include "Tests/Gameplay/Fake/GeoTestEnemy.h"
#include "Tests/Gameplay/Fake/GeoTestPatternAbility.h"
#include "Tests/Gameplay/Fake/GeoTestWorld.h"
#include "Tests/Gameplay/GeoTestFlags.h"
#include "Tool/CatLine/GeoCatLineBank.h"
#include "Tool/GeoDifficulty.h"
#include "Tool/UGeoGameplayLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoCharactersSetupHealthResetTest, "GeoTrinity.Characters.SetupHealthReset",
								 GeoCharactersTestFlags)

bool FGeoCharactersSetupHealthResetTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	World->SetGameInstance(NewObject<UGameInstance>(GEngine));
	AddExpectedError(TEXT("Missing DefaultAttributes"), EAutomationExpectedErrorFlags::Contains, 0);
	World->SetGameState(World->SpawnActor<AGeoGameState>());

	FGeoEnemySetup Setup;
	Setup.bResetToFullLife = true;
	Setup.MaxHealth = FScalableFloat(200.f);
	FTransform const Transform;
	AEnemyCharacter* const Enemy =
		World->SpawnActorDeferred<AEnemyCharacter>(AEnemyCharacter::StaticClass(), Transform);
	Enemy->ApplySetup(Setup);
	Enemy->FinishSpawning(Transform);

	UGeoAbilitySystemComponent* const ASC = GeoASLib::GetGeoAscFromActor(Enemy);
	ASC->SetNumericAttributeBase(UGeoAttributeSetBase::GetHealthAttribute(), 0.f);
	TestFalse(TEXT("It does not die at zero"), Enemy->IsDead());
	TestEqual(TEXT("It is back at the overridden max health"),
			  ASC->GetNumericAttributeBase(UGeoAttributeSetBase::GetHealthAttribute()), 200.f);

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoCharactersSetupAggroRadiusTest, "GeoTrinity.Characters.SetupAggroRadius",
								 GeoCharactersTestFlags)

bool FGeoCharactersSetupAggroRadiusTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AEnemyCharacter* const Enemy =
		World->SpawnActorDeferred<AEnemyCharacter>(AEnemyCharacter::StaticClass(), FTransform());
	TestTrue(TEXT("No setup keeps the controller's radius"), Enemy->AggroRadiusOverride < 0.f);

	FGeoEnemySetup Setup;
	TestTrue(TEXT("A setup keeps the controller's radius by default"), Setup.AggroRadius < 0.f);
	Setup.AggroRadius = 100.f;
	Enemy->ApplySetup(Setup);
	TestEqual(TEXT("The setup's radius reaches the enemy"), Enemy->AggroRadiusOverride, 100.f);

	Enemy->Destroy();
	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoCharactersTeamReplicatedTest, "GeoTrinity.Characters.TeamReplicated",
								 GeoCharactersTestFlags)

bool FGeoCharactersTeamReplicatedTest::RunTest(FString const& /*Parameters*/)
{
	for (UClass* const CharacterClass : {AEnemyCharacter::StaticClass(), APlayableCharacter::StaticClass()})
	{
		FProperty const* const TeamProperty = CharacterClass->FindPropertyByName("TeamId");
		if (TestNotNull(TEXT("TeamId exists"), TeamProperty))
		{
			TArray<FLifetimeProperty> Replicated;
			CharacterClass->GetDefaultObject()->GetLifetimeReplicatedProps(Replicated);
			bool const bRegistered = Replicated.ContainsByPredicate(
				[TeamProperty](FLifetimeProperty const& Lifetime)
				{
					return Lifetime.RepIndex == TeamProperty->RepIndex;
				});
			TestTrue(*FString::Printf(TEXT("%s replicates TeamId, so a client judges allies like the server"),
									  *CharacterClass->GetName()),
					 bRegistered);
		}
	}
	return true;
}

#endif
