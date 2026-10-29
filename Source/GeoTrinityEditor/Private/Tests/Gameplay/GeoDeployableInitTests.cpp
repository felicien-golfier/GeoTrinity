// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "GameClasses/Match/GeoGameState.h"
#include "Misc/AutomationTest.h"
#include "Tests/Gameplay/Fake/GeoTestDeployable.h"
#include "Tests/Gameplay/Fake/GeoTestWorld.h"
#include "Tool/Team.h"

#if WITH_DEV_AUTOMATION_TESTS

constexpr EAutomationTestFlags GeoDeployableInitTestFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoDeployableInitPhaseTest, "GeoTrinity.Gameplay.Deployable.InitPhase",
								 GeoDeployableInitTestFlags)

bool FGeoDeployableInitPhaseTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	World->SetGameInstance(NewObject<UGameInstance>(GEngine));
	World->SetGameState(World->SpawnActor<AGeoGameState>());

	AGeoTestDeployer* const Deployer = World->SpawnActor<AGeoTestDeployer>();

	FDeployableData Data;
	Data.Owner = Deployer;
	Data.Instigator = Deployer;
	Data.Params.InitDuration = 5.f;
	Data.Params.bAutoFinishInit = false;
	AGeoDeployableBase* const Deployable =
		GeoASLib::FullySpawnDeployable(AGeoTestDeployable::StaticClass(), Data, FTransform::Identity);

	if (TestNotNull(TEXT("The deployable spawns"), Deployable))
	{
		TestTrue(TEXT("It starts arriving"), Deployable->IsInitialising());
		TestFalse(TEXT("Arriving: not damageable"), Deployable->CanBeDamaged());
		TestFalse(TEXT("Arriving: no collision"), Deployable->GetActorEnableCollision());

		for (int32 Frame = 0; Frame < 15; ++Frame)
		{
			World->Tick(LEVELTICK_All, 0.4f); // the engine caps a frame at 0.4 s
		}

		Deployable->Tick(0.f);

		TestTrue(TEXT("Released by its spawner only: still arriving after the duration"), Deployable->IsInitialising());

		Deployable->FinishInit();

		TestFalse(TEXT("Once released it is live"), Deployable->IsInitialising());
		TestTrue(TEXT("Live: damageable"), Deployable->CanBeDamaged());
		TestTrue(TEXT("Live: collision back"), Deployable->GetActorEnableCollision());
	}

	DestroyTestWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
