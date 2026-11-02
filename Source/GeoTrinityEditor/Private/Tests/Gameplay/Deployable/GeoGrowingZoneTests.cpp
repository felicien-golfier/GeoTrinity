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
#include "Encounter/Action/GeoSpawnZoneAction.h"
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
#include "Tool/CatLine/GeoCatLineBank.h"
#include "Tool/GeoDifficulty.h"
#include "Tool/UGeoGameplayLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

constexpr EAutomationTestFlags GeoDeployableTestFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoGrowingZoneTest, "GeoTrinity.Deployable.GrowingZone", GeoDeployableTestFlags)

bool FGeoGrowingZoneTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	World->SetGameInstance(NewObject<UGameInstance>(GEngine));
	AddExpectedError(TEXT("Missing DefaultAttributes"), EAutomationExpectedErrorFlags::Contains, 0);
	World->SetGameState(World->SpawnActor<AGeoGameState>());
	FDeployableData Data;
	Data.Params.Size = 100.f;
	Data.Params.Color.Color = EGeoColor::Heal;
	AGeoTestReadableZone* const FixedZone = Cast<AGeoTestReadableZone>(GeoASLib::FullySpawnDeployable(
		AGeoTestReadableZone::StaticClass(), Data, FTransform(FVector(2000.f, 0.f, 0.f)), World));
	Data.Owner = nullptr;
	Data.Instigator = nullptr;
	Data.Params.GrowToSize = FScalableFloat(600.f);
	Data.Params.GrowSeconds = FScalableFloat(10.f);
	AGeoTestReadableZone* const Zone = Cast<AGeoTestReadableZone>(
		GeoASLib::FullySpawnDeployable(AGeoTestReadableZone::StaticClass(), Data, FTransform::Identity, World));
	if (TestNotNull(TEXT("The zones spawn"), Zone) && FixedZone)
	{
		TestEqual(TEXT("It starts at its spawn size"), Zone->GetTestRadius(), 100.f);
		World->TimeSeconds += 5.f;
		Zone->Tick(0.f);
		FixedZone->Tick(0.f);
		TestEqual(TEXT("Half of GrowSeconds: half way to GrowToSize"), Zone->GetTestRadius(), 350.f, 0.01f);
		TestEqual(TEXT("The capsule follows"),
				  Zone->FindComponentByClass<UCapsuleComponent>()->GetUnscaledCapsuleRadius(), 350.f, 0.01f);
		TestEqual(TEXT("The replicated spawn size never changes"), Zone->GetSpawnSize(), 100.f);
		TestEqual(TEXT("GrowToSize 0: it keeps its size"), FixedZone->GetTestRadius(), 100.f);
		World->TimeSeconds += 60.f;
		Zone->Tick(0.f);
		TestEqual(TEXT("It stops at GrowToSize"), Zone->GetTestRadius(), 600.f, 0.01f);

		World->TimeSeconds = 100.f;
		Data.Owner = nullptr;
		Data.Instigator = nullptr;
		AGeoTestReadableZone* const LateZone = Cast<AGeoTestReadableZone>(
			GeoASLib::FullySpawnDeployable(AGeoTestReadableZone::StaticClass(), Data, FTransform::Identity, World));
		if (TestNotNull(TEXT("The late zone spawns"), LateZone))
		{
			TestEqual(TEXT("The growth starts at the server time of its BeginPlay"),
					  LateZone->GetGrowthStartServerTime(), 100.f);
			LateZone->ReceiveGrowthStartServerTime(92.f);
			World->TimeSeconds = 97.f;
			LateZone->Tick(0.f);
			TestEqual(TEXT("A late receiver grows from the server's start, not its own BeginPlay"),
					  LateZone->GetTestRadius(), 350.f, 0.01f);
		}
	}

	DestroyTestWorld(World);
	return true;
}

#endif
