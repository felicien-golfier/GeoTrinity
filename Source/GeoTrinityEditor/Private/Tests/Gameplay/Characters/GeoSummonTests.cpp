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
#include "Tests/Gameplay/Encounter/GeoEncounterTestHelpers.h"
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoSummonedDummyTest, "GeoTrinity.Characters.SummonedDummy", GeoCharactersTestFlags)

bool FGeoSummonedDummyTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AGeoTestCasterArena* const Arena = SpawnCasterArena(*this, *World);
	TSoftClassPtr<AGeoEffectZone> const ProjectZoneClass = GetDefault<UGameDataSettings>()->DefaultZoneClass;
	GetMutableDefault<UGameDataSettings>()->DefaultZoneClass = AGeoTestReadableZone::StaticClass();
	AEnemyCharacter* const Dummy =
		Arena->SpawnAdd(AGeoTestSummonedDummy::StaticClass(), FTransform(FVector(300.f, 0.f, 0.f)));
	if (TestNotNull(TEXT("The dummy spawns"), Dummy))
	{
		TActorIterator<AGeoTestReadableZone> const Zone(World);
		if (TestTrue(TEXT("The default zone spawns at its feet"), static_cast<bool>(Zone)))
		{
			World->TimeSeconds += 5.f;
			Zone->Tick(0.f);
			TestEqual(TEXT("It grows by the dummy's ZoneParams"), Zone->GetTestRadius(), 350.f, 0.01f);
		}

		Dummy->Death();
		AEnemyCharacter* Ally = nullptr;
		for (TActorIterator<AEnemyCharacter> It(World); It && !Ally; ++It)
		{
			Ally = It->ActorHasTag(TEXT("Ally")) ? *It : nullptr;
		}

		if (TestNotNull(TEXT("Killed, it leaves its ally"), Ally))
		{
			TestTrue(TEXT("An add of the same arena"), Ally->Arena.Get() == Arena);
			TestEqual(TEXT("With the ally setup's team"), Ally->GetGenericTeamId(),
					  FGenericTeamId(static_cast<uint8>(ETeam::Player)));
			TestEqual(TEXT("In its place"), Ally->GetActorLocation().X, 300.0, 1.0);
		}
	}

	GetMutableDefault<UGameDataSettings>()->DefaultZoneClass = ProjectZoneClass;
	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoSummonAddSetupTest, "GeoTrinity.Abilities.SummonAddSetup", GeoCharactersTestFlags)

bool FGeoSummonAddSetupTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AddExpectedError(TEXT("The State Tree schema is not ready to run"), EAutomationExpectedErrorFlags::Contains, 0);
	AGeoTestCasterArena* const Arena = SpawnCasterArena(*this, *World);
	AEnemyCharacter* const Boss = Arena->GetBoss();
	UPattern* const Pattern = GeoASLib::GetGeoAscFromActor(Boss)->CreatePatternInstance(
		UGeoTestSpawnAddPattern::StaticClass(), FGameplayTag());
	auto const FindAdds = [World](FName const Tag)
	{
		TArray<AEnemyCharacter*> Adds;
		for (TActorIterator<AEnemyCharacter> It(World); It; ++It)
		{
			if (It->ActorHasTag(Tag))
			{
				Adds.Add(*It);
			}
		}

		return Adds;
	};

	LaunchTestPattern(*Pattern, Boss, {FVector2D(400.f, 0.f)});
	TestEqual(TEXT("No add before the hit"), FindAdds(TEXT("Summoned")).Num(), 0);
	Pattern->EndPattern(true);
	TestEqual(TEXT("Force-stop: nothing spawned"), FindAdds(TEXT("Summoned")).Num(), 0);

	LaunchTestPattern(*Pattern, Boss, {FVector2D(400.f, 0.f), FVector2D(-400.f, 0.f)});
	Pattern->EndPattern();
	TArray<AEnemyCharacter*> const Adds = FindAdds(TEXT("Summoned"));
	if (TestEqual(TEXT("Natural end: one add per location, with AddSetup"), Adds.Num(), 2))
	{
		TestTrue(TEXT("Adds of the caster's arena"), Arena->GetLiveAdds().Contains(Adds[0]));
		TestEqual(TEXT("With the setup's max health"),
				  GeoASLib::GetGeoAscFromActor(Adds[0])->GetNumericAttributeBase(
					  UGeoAttributeSetBase::GetMaxHealthAttribute()),
				  300.f);
		TestEqual(TEXT("On their locations"), FMath::Abs(Adds[0]->GetActorLocation().X), 400.0, 1.0);
		TestEqual(TEXT("Each its own"), Adds[0]->GetActorLocation().X + Adds[1]->GetActorLocation().X, 0.0, 1.0);
	}

	LaunchTestPattern(*Pattern, Boss, {FVector2D(0.f, 400.f)}, GeoDifficultyMask::Safe);
	Pattern->EndPattern();
	TArray<AEnemyCharacter*> const AllAdds = FindAdds(TEXT("Summoned"));
	if (TestEqual(TEXT("Safe level: the add is still AddSetup's"), AllAdds.Num(), 3))
	{
		int32 AddsWithTree = 0;
		for (AEnemyCharacter const* const Add : AllAdds)
		{
			AddsWithTree += Add->StateTree != nullptr ? 1 : 0;
		}

		TestEqual(TEXT("Safe level: only that add loses AddSetup's StateTree"), AddsWithTree, 2);
	}

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoArenaBossSetupTest, "GeoTrinity.Arena.BossSetup", GeoCharactersTestFlags)

bool FGeoArenaBossSetupTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	World->SetGameInstance(NewObject<UGameInstance>(GEngine));
	AddExpectedError(TEXT("Missing DefaultAttributes"), EAutomationExpectedErrorFlags::Contains, 0);
	World->SetGameState(World->SpawnActor<AGeoGameState>());
	FGameplayTag const ArenaTag = UGameplayTagsManager::Get().RequestGameplayTag(TEXT("Arena.Hex"));
	AGeoTargetPoint* const AddPoint = World->SpawnActor<AGeoTargetPoint>(FVector(500.f, 0.f, 0.f), FRotator());
	AddPoint->GameplayTags.AddTag(FGeoGameplayTags::Get().TargetPoint_AddSpawn);
	AddPoint->GameplayTags.AddTag(ArenaTag);

	FGameplayTag const AbilityTag = UGeoTestCastAbility::GetCastTag();
	AGeoTestCasterArena* const Arena =
		World->SpawnActorDeferred<AGeoTestCasterArena>(AGeoTestCasterArena::StaticClass(), FTransform::Identity);
	Arena->ArenaTag = ArenaTag;
	FGeoEnemySetup& BossSetup = Arena->EditBossSetup();
	BossSetup.Team = ETeam::Neutral;
	BossSetup.AbilityTags = {AbilityTag};
	BossSetup.MaxHealth = FScalableFloat(500.f);
	BossSetup.BehaviourStateTree = NewObject<UStateTree>(Arena);
	FGeoArenaAdd& StartAdd = Arena->EditStartAdds().AddDefaulted_GetRef();
	StartAdd.Class = AEnemyCharacter::StaticClass();
	StartAdd.Setup.Team = ETeam::Player;
	StartAdd.Setup.Tag = TEXT("StartAdd");
	Arena->FinishSpawning(FTransform::Identity);

	auto const TestBoss = [this, Arena, AbilityTag](TCHAR const* const When)
	{
		AEnemyCharacter* const Boss = Arena->GetBoss();
		if (TestNotNull(When, Boss))
		{
			UGeoAbilitySystemComponent const* const ASC = GeoASLib::GetGeoAscFromActor(Boss);
			TestEqual(TEXT("The setup's team"), Boss->GetGenericTeamId(),
					  FGenericTeamId(static_cast<uint8>(ETeam::Neutral)));
			TestTrue(TEXT("The setup's abilities"), ASC->StartupAbilityTags.Contains(AbilityTag));
			TestEqual(TEXT("The setup's max health"),
					  ASC->GetNumericAttributeBase(UGeoAttributeSetBase::GetMaxHealthAttribute()), 500.f);
			TestTrue(TEXT("The setup's behaviour tree"),
					 Boss->BehaviourStateTree == Arena->EditBossSetup().BehaviourStateTree);
			TestTrue(TEXT("Still the arena's boss"), Boss->Arena.Get() == Arena);
		}

		TArray<AEnemyCharacter const*> const Adds = Arena->GetLiveAdds();
		if (TestEqual(TEXT("One start add"), Adds.Num(), 1))
		{
			TestTrue(TEXT("With its setup's tag"), Adds[0]->ActorHasTag(TEXT("StartAdd")));
			TestEqual(TEXT("With its setup's team"), Adds[0]->GetGenericTeamId(),
					  FGenericTeamId(static_cast<uint8>(ETeam::Player)));
			TestEqual(TEXT("On its add point"), Adds[0]->GetActorLocation().X, 500.0, 1.0);
		}
	};
	TestBoss(TEXT("The arena spawns its boss"));
	Arena->ResetBoss();
	TestBoss(TEXT("A reset spawns it again"));

	DestroyTestWorld(World);
	return true;
}

#endif
