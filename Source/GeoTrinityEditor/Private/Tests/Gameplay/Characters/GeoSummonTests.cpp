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
#include "Tool/UGeoGameplayLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoSummonedDummyTest, "GeoTrinity.Characters.SummonedDummy", GeoCharactersTestFlags)

bool FGeoSummonedDummyTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AGeoTestCasterArena* const Arena = SpawnCasterArena(*this, *World);
	FGameplayAbilitySpecHandle FirstAllyHandle;
	auto const SpawnDummy =
		[Arena, &FirstAllyHandle](float const X, TSubclassOf<UGeoSpawnEnemyCharacterAbility> const AllyAbility)
	{
		AEnemyCharacter* const Dummy =
			Arena->SpawnAdd(AEnemyCharacter::StaticClass(), FTransform(FVector(X, 0.f, 0.f)));
		UGeoAbilitySystemComponent* const ASC = GeoASLib::GetGeoAscFromActor(Dummy);
		FGameplayAbilitySpecHandle const Zone =
			ASC->GiveAbility(FGameplayAbilitySpec(UGeoTestSelfZoneAbility::StaticClass(), 1));
		FGameplayAbilitySpecHandle const Ally = ASC->GiveAbility(FGameplayAbilitySpec(AllyAbility, 1));
		FirstAllyHandle = FirstAllyHandle.IsValid() ? FirstAllyHandle : Ally;
		ASC->TryActivateAbility(Zone);
		return Dummy;
	};
	auto const FindAllies = [World]()
	{
		TArray<AEnemyCharacter*> Allies;
		for (TActorIterator<AEnemyCharacter> It(World); It; ++It)
		{
			if (It->ActorHasTag(TEXT("Ally")))
			{
				Allies.Add(*It);
			}
		}

		return Allies;
	};
	int32 AlliesSeenByListener = 0;
	World->GetGameState<AGeoGameState>()->OnCharacterDied.AddLambda(
		[&AlliesSeenByListener, &FindAllies](AGeoCharacter&)
		{
			AlliesSeenByListener = FindAllies().Num();
		});

	AEnemyCharacter* const Dummy = SpawnDummy(300.f, UGeoTestSpawnAllyAbility::StaticClass());
	AEnemyCharacter* const OtherDummy = SpawnDummy(-300.f, UGeoTestSpawnAllyAbility::StaticClass());
	TickTestTimers(*World, 0.f);
	TickTestTimers(*World, 5.f); // The zone is placed when the ability's fire delay ends.
	TActorIterator<AGeoTestDeployable> const FirstZone(World);
	if (TestTrue(TEXT("Its zone is placed"), static_cast<bool>(FirstZone)))
	{
		TestEqual(TEXT("Centred on the dummy, not on a fire socket"), FMath::Abs(FirstZone->GetActorLocation().X),
				  300.0, 1.0);
	}

	TestEqual(TEXT("Alive: no ally yet"), FindAllies().Num(), 0);
	FGameplayAbilitySpec const* const AllySpec =
		GeoASLib::GetGeoAscFromActor(Dummy)->FindAbilitySpecFromHandle(FirstAllyHandle);
	TestFalse(TEXT("The ally ability is not active while it waits"), AllySpec && AllySpec->IsActive());
	Dummy->Death();
	TArray<AEnemyCharacter*> const Allies = FindAllies();
	if (TestEqual(TEXT("A dead dummy leaves one ally"), Allies.Num(), 1))
	{
		TestTrue(TEXT("An add of the same arena"), Allies[0]->Arena.Get() == Arena);
		TestEqual(TEXT("With the ally setup's team"), Allies[0]->GetGenericTeamId(),
				  FGenericTeamId(static_cast<uint8>(ETeam::Player)));
		TestEqual(TEXT("In its place"), Allies[0]->GetActorLocation().X, 300.0, 1.0);
	}

	TestEqual(TEXT("A death listener already sees the ally"), AlliesSeenByListener, 1);
	OtherDummy->Death();
	TestEqual(TEXT("Each dummy leaves its own ally"), FindAllies().Num(), 2);

	AEnemyCharacter* const Wiped = SpawnDummy(0.f, UGeoTestSpawnAllyAbility::StaticClass());
	Wiped->Destroy();
	TestEqual(TEXT("Destroyed without dying: no ally"), FindAllies().Num(), 2);

	AEnemyCharacter* const Silent = SpawnDummy(100.f, UGeoTestSpawnAllyOffAbility::StaticClass());
	Silent->Death();
	TestEqual(TEXT("Without bSpawnOnOwnerDying: no ally"), FindAllies().Num(), 2);

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoSpawnEnemyCharacterTest, "GeoTrinity.Abilities.SpawnEnemyCharacter",
								 GeoCharactersTestFlags)

bool FGeoSpawnEnemyCharacterTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AddExpectedError(TEXT("The State Tree schema is not ready to run"), EAutomationExpectedErrorFlags::Contains, 0);
	AGeoTestCasterArena* const Arena = SpawnCasterArena(*this, *World);
	AEnemyCharacter* const Boss = Arena->GetBoss();
	UGeoAbilitySystemComponent* const ASC = GeoASLib::GetGeoAscFromActor(Boss);
	UPattern* const Pattern = ASC->CreatePatternInstance(UGeoTestZonePattern::StaticClass(), FGameplayTag());
	FGameplayAbilitySpecHandle const Spawn =
		ASC->GiveAbility(FGameplayAbilitySpec(UGeoTestSpawnEnemyAbility::StaticClass(), 1));
	auto const FindAdds = [World]()
	{
		TArray<AEnemyCharacter*> Adds;
		for (TActorIterator<AEnemyCharacter> It(World); It; ++It)
		{
			if (It->ActorHasTag(TEXT("Summoned")))
			{
				Adds.Add(*It);
			}
		}

		return Adds;
	};

	LaunchTestPattern(*Pattern, Boss, {FVector2D(400.f, 0.f), FVector2D(-400.f, 0.f)});
	Pattern->EndPattern(true);
	ASC->TryActivateAbility(Spawn);
	TArray<AEnemyCharacter*> const Adds = FindAdds();
	if (TestEqual(TEXT("One enemy per target location"), Adds.Num(), 2))
	{
		TestTrue(TEXT("Adds of the caster's arena"), Arena->GetLiveAdds().Contains(Adds[0]));
		TestEqual(TEXT("With the setup's max health"),
				  GeoASLib::GetGeoAscFromActor(Adds[0])->GetNumericAttributeBase(
					  UGeoAttributeSetBase::GetMaxHealthAttribute()),
				  300.f);
		TestEqual(TEXT("On their locations"), FMath::Abs(Adds[0]->GetActorLocation().X), 400.0, 1.0);
		TestEqual(TEXT("Each its own"), Adds[0]->GetActorLocation().X + Adds[1]->GetActorLocation().X, 0.0, 1.0);
	}

	FGameplayAbilitySpec const* const Spec = ASC->FindAbilitySpecFromHandle(Spawn);
	TestFalse(TEXT("The ability ended in the same call"), Spec && Spec->IsActive());

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoPatternTargetsTest, "GeoTrinity.Abilities.PatternTargets", GeoCharactersTestFlags)

bool FGeoPatternTargetsTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	PrepareEnemyWorld(*this, *World);
	AGeoTestCasterArena* const Arena = World->SpawnActor<AGeoTestCasterArena>();
	AEnemyCharacter* const Boss = Arena->GetBoss();
	UGeoAbilitySystemComponent* const ASC = GeoASLib::GetGeoAscFromActor(Boss);
	UPattern* const Pattern = ASC->CreatePatternInstance(UGeoTestZonePattern::StaticClass(), FGameplayTag());
	UPattern* const OtherPattern = ASC->CreatePatternInstance(UGeoTestPillarPattern::StaticClass(), FGameplayTag());
	FGeoTarget Target;
	Target.Mode = EGeoTarget::PatternTargets;
	Target.PatternClass = UGeoTestZonePattern::StaticClass();
	FGeoTargetContext Context;
	Context.Caster = Boss;

	TestEqual(TEXT("Never launched: no target"), GeoASLib::GetTargetLocations(Target, Context).Num(), 0);

	LaunchTestPattern(*Pattern, Boss, {FVector2D(400.f, 0.f), FVector2D(0.f, 300.f)});
	Pattern->EndPattern(true);
	LaunchTestPattern(*OtherPattern, Boss, {FVector2D(0.f, -900.f)});
	OtherPattern->EndPattern(true);
	Boss->SetActorLocation(FVector(-800.f, 0.f, Boss->GetActorLocation().Z));
	TArray<FVector2D> const Locations = GeoASLib::GetTargetLocations(Target, Context);
	if (TestEqual(TEXT("After the pattern ended: its launch's locations"), Locations.Num(), 2))
	{
		TestEqual(TEXT("Fixed, the caster moved"), Locations[0].X, 400.0, 1.0);
		TestEqual(TEXT("Another pattern does not interfere"), Locations[1].Y, 300.0, 1.0);
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

	// A real non-passive spell: the boss really gets it. The fake cast tag is ShieldBurst, a passive that now
	// activates at InitGAS and cannot on this mesh-less boss.
	FGameplayTag const AbilityTag = UGameplayTagsManager::Get().RequestGameplayTag(TEXT("Ability.Spell.BasicBeam"));
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
