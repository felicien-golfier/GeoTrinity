// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Engine/GameInstance.h"
#include "GameClasses/Match/GeoGameState.h"
#include "Misc/AutomationTest.h"
#include "Tests/Gameplay/Fake/GeoTestEnemy.h"
#include "Tests/Gameplay/Fake/GeoTestTurret.h"
#include "Tests/Gameplay/Fake/GeoTestWorld.h"
#include "Tool/Team.h"

#if WITH_DEV_AUTOMATION_TESTS

constexpr EAutomationTestFlags GeoTurretTestFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoTurretWaitsForTargetTest, "GeoTrinity.Gameplay.Turret.WaitsForTarget",
								 GeoTurretTestFlags)

bool FGeoTurretWaitsForTargetTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	World->SetGameInstance(NewObject<UGameInstance>(GEngine));
	AddExpectedError(TEXT("Missing DefaultAttributes"), EAutomationExpectedErrorFlags::Contains, 0);
	World->SetGameState(World->SpawnActor<AGeoGameState>());

	AGeoTestBodyEnemy* const Owner = World->SpawnActor<AGeoTestBodyEnemy>();
	Owner->SetGenericTeamId(FGenericTeamId(static_cast<uint8>(ETeam::Player)));
	AGeoTestBodyEnemy* const Enemy = World->SpawnActor<AGeoTestBodyEnemy>();

	FDeployableData Data;
	Data.Owner = Owner;
	Data.Instigator = Owner;
	Data.Params.LifeDrainMaxDuration = 10.f;
	AGeoTestTurret const* const Turret =
		Cast<AGeoTestTurret>(GeoASLib::FullySpawnDeployable(AGeoTestTurret::StaticClass(), Data, FTransform::Identity));
	UGeoAbilitySystemComponent* const OwnerASC = GeoASLib::GetGeoAscFromActor(Owner);

	if (TestNotNull(TEXT("The turret spawns"), Turret) && TestNotNull(TEXT("The owner has an ASC"), OwnerASC))
	{
		TestNull(TEXT("A hostile in range but no basic hit yet: the turret waits"), Turret->GetBestTarget());

		OwnerASC->SetLastBasicAbilityTarget(Enemy);
		TestEqual(TEXT("After the owner's basic hit the turret follows that target"), Turret->GetBestTarget(),
				  static_cast<AActor*>(Enemy));

		Enemy->Destroy();
		TestNull(TEXT("The target gone: the turret idles, no nearest fallback"), Turret->GetBestTarget());
	}

	DestroyTestWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
