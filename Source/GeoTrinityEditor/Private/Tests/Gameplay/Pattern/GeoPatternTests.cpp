// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/Hazard/LateralWavePattern.h"
#include "AbilitySystem/Abilities/Pattern/Hazard/SafeSpot/SafeSpot.h"
#include "AbilitySystem/Abilities/Pattern/Hazard/SafeSpot/SafeSpotMask.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Deployable/Zones/GeoEffectZone.h"
#include "Characters/PlayableCharacter.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "GameClasses/Match/GeoGameState.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Settings/GameDataSettings.h"
#include "Tests/Gameplay/Fake/GeoTestEncounter.h"
#include "Tests/Gameplay/Fake/GeoTestPatternAbility.h"
#include "Tests/Gameplay/Fake/GeoTestWorld.h"

#if WITH_DEV_AUTOMATION_TESTS

constexpr EAutomationTestFlags GeoPatternTestFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoSafeSpotShadowTest, "GeoTrinity.Pattern.SafeSpotShadow", GeoPatternTestFlags)

bool FGeoSafeSpotShadowTest::RunTest(FString const& /*Parameters*/)
{
	FVector2D const Origin(0.f, 0.f);
	FVector2D const Forward(1.f, 0.f);

	FSafeSpot const Pillar{FVector2D(500.f, 0.f), 100.f, ESafeSpotShadow::Behind};
	TestFalse(TEXT("Behind: before the spot is not shielded"), Pillar.Shields(FVector2D(300.f, 0.f), Origin, Forward));
	TestTrue(TEXT("Behind: behind the spot, within its radius, is shielded"),
			 Pillar.Shields(FVector2D(900.f, 50.f), Origin, Forward));
	TestFalse(TEXT("Behind: beside the spot's shadow is not shielded"),
			  Pillar.Shields(FVector2D(900.f, 300.f), Origin, Forward));

	FSafeSpot const Lane{FVector2D(500.f, 200.f), 150.f, ESafeSpotShadow::Lane};
	TestTrue(TEXT("Lane: before the spot, in its width, is shielded"),
			 Lane.Shields(FVector2D(100.f, 250.f), Origin, Forward));
	TestTrue(TEXT("Lane: behind the spot, in its width, is shielded"),
			 Lane.Shields(FVector2D(900.f, 100.f), Origin, Forward));
	TestFalse(TEXT("Lane: outside the spot's width is not shielded"),
			  Lane.Shields(FVector2D(500.f, 400.f), Origin, Forward));

	FSafeSpot const Disc{FVector2D(500.f, 200.f), 150.f, ESafeSpotShadow::Disc};
	FVector2D const Ignored(0.f, 1.f);
	TestTrue(TEXT("Disc: inside the radius is shielded"), Disc.Shields(FVector2D(600.f, 300.f), Ignored, Ignored));
	TestFalse(TEXT("Disc: outside the radius is not shielded"),
			  Disc.Shields(FVector2D(700.f, 200.f), Ignored, Ignored));
	TestFalse(TEXT("Disc: nothing behind it is shielded"), Disc.Shields(FVector2D(900.f, 200.f), Origin, Forward));
	TestFalse(TEXT("Disc: nothing along its lane is shielded"), Disc.Shields(FVector2D(500.f, 400.f), Origin, Forward));
	TestTrue(TEXT("A spot casts a Disc unless told otherwise"), FSafeSpot().Shadow == ESafeSpotShadow::Disc);

	TestFalse(TEXT("No spots shield nothing"), FSafeSpot::AnyShields({}, FVector2D(900.f, 0.f), Origin, Forward));
	TestTrue(TEXT("Any spot shielding is enough"),
			 FSafeSpot::AnyShields({Lane, Pillar}, FVector2D(900.f, 0.f), Origin, Forward));

	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoSafeSpotMaskTest, "GeoTrinity.Pattern.SafeSpotMask", GeoPatternTestFlags)

bool FGeoSafeSpotMaskTest::RunTest(FString const& /*Parameters*/)
{
	TArray<FLinearColor> SlotValues = FSafeSpotMask::GetSlotValues({});
	TestEqual(TEXT("Always one value per slot"), SlotValues.Num(), FSafeSpotMask::MaxSlots);
	TestEqual(TEXT("No spot: every slot holds the sentinel"), SlotValues[0].R, -10000.f);
	TestEqual(TEXT("No spot: the last slot too"), SlotValues.Last().R, -10000.f);
	TestEqual(TEXT("No spot: slots are free (alpha 0)"), SlotValues[0].A, 0.f);

	TArray<FSafeSpot> Spots;
	for (int32 Index = 0; Index < FSafeSpotMask::MaxSlots + 2; ++Index)
	{
		Spots.Add({FVector2D(Index * 100.f, 0.f), 50.f, ESafeSpotShadow::Behind});
	}

	AddExpectedMessage(TEXT("safe spots for"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	SlotValues = FSafeSpotMask::GetSlotValues(Spots);
	TestEqual(TEXT("Spots over the slots are skipped"), SlotValues.Num(), FSafeSpotMask::MaxSlots);
	TestEqual(TEXT("A spot lands in its slot"), SlotValues[3].R, 300.f);
	TestEqual(TEXT("The last slot is the eighth spot"), SlotValues.Last().R, 700.f);
	TestEqual(TEXT("A used slot has alpha 1"), SlotValues[3].A, 1.f);

	UWorld* const World = CreateTestWorld();
	FSafeSpotMask::Set(World, nullptr, Spots, FVector2D(1.f, 0.f), FVector2D::ZeroVector);
	FSafeSpotMask::Clear(World, nullptr);
	DestroyTestWorld(World);

	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoLateralWaveRollTest, "GeoTrinity.Pattern.LateralWaveRoll", GeoPatternTestFlags)

bool FGeoLateralWaveRollTest::RunTest(FString const& /*Parameters*/)
{
	TSet<float> Reached;
	for (int32 Seed = 0; Seed < 200; ++Seed)
	{
		float const Yaw = ULateralWavePattern::RollTravelYaw(Seed);
		TestEqual(TEXT("The same seed rolls the same yaw"), ULateralWavePattern::RollTravelYaw(Seed), Yaw);
		TestTrue(TEXT("A roll is one of the four basic directions"),
				 Yaw == 0.f || Yaw == 90.f || Yaw == 180.f || Yaw == 270.f);
		Reached.Add(Yaw);
	}

	TestEqual(TEXT("All four directions are reachable"), Reached.Num(), 4);
	TestFalse(TEXT("A firing keeps its fixed direction unless asked"), FLateralWavePatternData().bRandomDirection);

	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoSalveAimTest, "GeoTrinity.Pattern.SalveAim", GeoPatternTestFlags)

bool FGeoSalveAimTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	World->SetGameInstance(NewObject<UGameInstance>(GEngine));
	AddExpectedError(TEXT("Missing DefaultAttributes"), EAutomationExpectedErrorFlags::Contains, 0);
	World->SetGameState(World->SpawnActor<AGeoGameState>());
	for (FVector const& Location : {FVector(1000.f, 0.f, 0.f), FVector(-1000.f, 0.f, 0.f)})
	{
		FTransform const Transform(Location);
		APlayableCharacter* const Player =
			World->SpawnActorDeferred<APlayableCharacter>(APlayableCharacter::StaticClass(), Transform);
		Player->SetActorHiddenInGame(true);
		Player->FinishSpawning(Transform);
		World->SpawnActor<APlayerController>()->SetPawn(Player);
	}

	AGeoTestCasterArena* const Arena =
		World->SpawnActor<AGeoTestCasterArena>(FVector(0.f, 1000.f, 0.f), FRotator::ZeroRotator);
	auto const GetAimYaws = [](AActor* const Caster, EGeoTarget const Mode)
	{
		UGeoAbilitySystemComponent* const ASC = GeoASLib::GetGeoAscFromActor(Caster);
		FGameplayAbilitySpec const* Spec = ASC->FindAbilitySpecFromClass(UGeoTestSalveAbility::StaticClass());
		if (!Spec)
		{
			Spec = ASC->FindAbilitySpecFromHandle(
				ASC->GiveAbility(FGameplayAbilitySpec(UGeoTestSalveAbility::StaticClass(), 1)));
		}
		TArray<float> Yaws;
		for (FVector2D const& Location :
			 CastChecked<UGeoTestSalveAbility>(Spec->GetPrimaryInstance())->GetAimLocations(Mode))
		{
			Yaws.Add(static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Location.Y, Location.X))));
		}
		return Yaws;
	};

	TArray<float> const PlayerYaws = GetAimYaws(Arena->GetBoss(), EGeoTarget::AllPlayers);
	if (TestEqual(TEXT("AllPlayers: one yaw per live player"), PlayerYaws.Num(), 2))
	{
		TestTrue(TEXT("At each player"), FMath::IsNearlyEqual(FMath::Abs(PlayerYaws[0] - PlayerYaws[1]), 180.f, 0.1f));
	}

	FGeoEnemySetup AllySetup;
	AllySetup.Team = ETeam::Player;
	AEnemyCharacter* const Ally = Arena->SpawnAdd(AEnemyCharacter::StaticClass(), FTransform(), &AllySetup);
	TestEqual(TEXT("Boss, no fight: none"), GetAimYaws(Ally, EGeoTarget::Boss).Num(), 0);

	Arena->StartFight();
	TArray<float> const BossYaws = GetAimYaws(Ally, EGeoTarget::Boss);
	if (TestEqual(TEXT("Boss, fighting: one yaw"), BossYaws.Num(), 1))
	{
		TestEqual(TEXT("At the fighting arena's boss"), BossYaws[0], 90.f, 0.1f);
	}

	Arena->GetBoss()->Death();
	TestEqual(TEXT("Boss dead: none"), GetAimYaws(Ally, EGeoTarget::Boss).Num(), 0);

	DestroyTestWorld(World);
	return true;
}

/** A world with a game state and a caster arena, for a pattern cast by its boss. */
static AGeoTestCasterArena* PrepareZoneWorld(FAutomationTestBase& Test, UWorld& World)
{
	World.SetGameInstance(NewObject<UGameInstance>(GEngine));
	Test.AddExpectedError(TEXT("Missing DefaultAttributes"), EAutomationExpectedErrorFlags::Contains, -1);
	World.SetGameState(World.SpawnActor<AGeoGameState>());
	return World.SpawnActor<AGeoTestCasterArena>();
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoZoneSeveralLocationsTest, "GeoTrinity.Pattern.ZoneSeveralLocations",
								 GeoPatternTestFlags)

bool FGeoZoneSeveralLocationsTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AEnemyCharacter* const Boss = PrepareZoneWorld(*this, *World)->GetBoss();
	UGeoTestZonePattern* const Pattern = CastChecked<UGeoTestZonePattern>(
		GeoASLib::GetGeoAscFromActor(Boss)->CreatePatternInstance(UGeoTestZonePattern::StaticClass(), FGameplayTag()));

	FVector2D const First(1000.f, 0.f);
	FVector2D const Second(-1000.f, 500.f);
	LaunchTestPattern(*Pattern, Boss, {First, Second});
	TestEqual(TEXT("One telegraph per location"), Pattern->GetTelegraphCount(), 2);
	TestTrue(TEXT("In the first circle"), Pattern->IsLocationInHazard(Boss, First + FVector2D(50.f, 0.f)));
	TestTrue(TEXT("In the second circle"), Pattern->IsLocationInHazard(Boss, Second + FVector2D(0.f, 50.f)));
	TestFalse(TEXT("Outside both"), Pattern->IsLocationInHazard(Boss, FVector2D(0.f, 3000.f)));

	static_cast<UPattern*>(Pattern)->EndPattern(true);
	TestEqual(TEXT("Force-stop drops every telegraph"), Pattern->GetTelegraphCount(), 0);

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoBeamSweepTest, "GeoTrinity.Pattern.BeamSweep", GeoPatternTestFlags)

bool FGeoBeamSweepTest::RunTest(FString const& /*Parameters*/)
{
	FVector2D const TargetLocation(0.f, 1000.f);
	float const AimYaw = 90.f;
	UGeoTestBeamPattern* const Pattern = NewObject<UGeoTestBeamPattern>();

	Pattern->Aim(80.f, 2, TargetLocation);
	TestEqual(TEXT("Even seed: starts past the aim"), Pattern->GetYawAtFraction(0.f), AimYaw + 80.f, 0.1f);
	TestEqual(TEXT("Even seed: halfway on the aim"), Pattern->GetYawAtFraction(0.5f), AimYaw, 0.1f);
	TestEqual(TEXT("Even seed: ends past the aim on the other side"), Pattern->GetYawAtFraction(1.f), AimYaw - 80.f,
			  0.1f);

	Pattern->Aim(80.f, 3, TargetLocation);
	TestEqual(TEXT("Odd seed: starts on the other side"), Pattern->GetYawAtFraction(0.f), AimYaw - 80.f, 0.1f);
	TestEqual(TEXT("Odd seed: halfway on the aim"), Pattern->GetYawAtFraction(0.5f), AimYaw, 0.1f);
	TestEqual(TEXT("Odd seed: ends on the first side"), Pattern->GetYawAtFraction(1.f), AimYaw + 80.f, 0.1f);

	Pattern->Aim(0.f, 2, TargetLocation);
	TestEqual(TEXT("No sweep: on the aim at the start"), Pattern->GetYawAtFraction(0.f), AimYaw, 0.1f);
	TestEqual(TEXT("No sweep: on the aim at the end"), Pattern->GetYawAtFraction(1.f), AimYaw, 0.1f);

	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoSpawnPillarPatternTest, "GeoTrinity.Pattern.SpawnPillar", GeoPatternTestFlags)

bool FGeoSpawnPillarPatternTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AEnemyCharacter* const Boss = PrepareZoneWorld(*this, *World)->GetBoss();
	UPattern* const Pattern =
		GeoASLib::GetGeoAscFromActor(Boss)->CreatePatternInstance(UGeoTestPillarPattern::StaticClass(), FGameplayTag());
	auto const FindPillars = [World](double const LocationY)
	{
		TArray<AGeoTestPillar*> Pillars;
		for (TActorIterator<AGeoTestPillar> It(World); It; ++It)
		{
			if (FMath::IsNearlyEqual(It->GetActorLocation().Y, LocationY, 1.0))
			{
				Pillars.Add(*It);
			}
		}

		return Pillars;
	};

	double const ReleasedY = 2000.0;
	LaunchTestPattern(*Pattern, Boss, {FVector2D(2000.f, ReleasedY), FVector2D(-2000.f, ReleasedY)});
	TArray<AGeoTestPillar*> const Released = FindPillars(ReleasedY);
	if (TestEqual(TEXT("One pillar per location"), Released.Num(), 2))
	{
		TestTrue(TEXT("During the telegraph: arriving"), Released[0]->IsInitialising());
		TestFalse(TEXT("During the telegraph: not damageable"), Released[0]->CanBeDamaged());
		Pattern->EndPattern();
		TestFalse(TEXT("Natural end: released"), Released[0]->IsInitialising() || Released[1]->IsInitialising());
		TestTrue(TEXT("Natural end: live and damageable"),
				 Released[0]->IsActive() && Released[0]->CanBeDamaged() && Released[1]->CanBeDamaged());
	}

	double const ExpiredY = -2000.0;
	LaunchTestPattern(*Pattern, Boss, {FVector2D(2000.f, ExpiredY)});
	TArray<AGeoTestPillar*> const Expired = FindPillars(ExpiredY);
	if (TestEqual(TEXT("One pillar on the location"), Expired.Num(), 1))
	{
		Pattern->EndPattern(true);
		TestFalse(TEXT("Force-stop: expired"), Expired[0]->IsActive());
	}

	DestroyTestWorld(World);
	return true;
}

/** How many of the test zones in World are still up. */
static int32 CountActiveZones(UWorld& World)
{
	int32 Count = 0;
	for (TActorIterator<AGeoTestReadableZone> It(&World); It; ++It)
	{
		Count += It->IsActive() ? 1 : 0;
	}

	return Count;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoZoneUntilStoppedTest, "GeoTrinity.Pattern.ZoneUntilStopped", GeoPatternTestFlags)

bool FGeoZoneUntilStoppedTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AEnemyCharacter* const Boss = PrepareZoneWorld(*this, *World)->GetBoss();
	TSoftClassPtr<AGeoEffectZone> const ProjectZoneClass = GetDefault<UGameDataSettings>()->DefaultZoneClass;
	GetMutableDefault<UGameDataSettings>()->DefaultZoneClass = AGeoTestReadableZone::StaticClass();
	UGeoTestLingeringZonePattern* const Pattern =
		CastChecked<UGeoTestLingeringZonePattern>(GeoASLib::GetGeoAscFromActor(Boss)->CreatePatternInstance(
			UGeoTestLingeringZonePattern::StaticClass(), FGameplayTag()));

	TestFalse(TEXT("Held until stopped with no life drain: not a burst"), Pattern->IsBurst());
	LaunchTestPattern(*Pattern, Boss, {FVector2D(1000.f, 0.f)});
	TickTestTimers(*World, 0.f);
	TickTestTimers(*World, 2.f);
	World->TimeSeconds += 2.f;
	TestEqual(TEXT("During the wind-up: no zone yet"), CountActiveZones(*World), 0);
	TestTrue(TEXT("During the wind-up: the pattern is alive"), Pattern->IsPatternActive());

	World->TimeSeconds += 3.01f;
	TickTestTimers(*World, 3.01f);
	TestEqual(TEXT("At the landing: the zone is up"), CountActiveZones(*World), 1);
	TestTrue(TEXT("At the landing: the pattern stays alive"), Pattern->IsPatternActive());

	static_cast<UPattern*>(Pattern)->EndPattern(true);
	TestEqual(TEXT("Force-stop recalls the zone"), CountActiveZones(*World), 0);
	TestFalse(TEXT("Force-stop ends the pattern"), Pattern->IsPatternActive());

	GetMutableDefault<UGameDataSettings>()->DefaultZoneClass = ProjectZoneClass;
	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoZoneJudgedAtLandingTest, "GeoTrinity.Pattern.ZoneJudgedAtLanding",
								 GeoPatternTestFlags)

bool FGeoZoneJudgedAtLandingTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AEnemyCharacter* const Boss = PrepareZoneWorld(*this, *World)->GetBoss();
	TSoftClassPtr<AGeoEffectZone> const ProjectZoneClass = GetDefault<UGameDataSettings>()->DefaultZoneClass;
	GetMutableDefault<UGameDataSettings>()->DefaultZoneClass = AGeoTestReadableZone::StaticClass();
	UGeoTestLingeringZonePattern* const Pattern =
		CastChecked<UGeoTestLingeringZonePattern>(GeoASLib::GetGeoAscFromActor(Boss)->CreatePatternInstance(
			UGeoTestLingeringZonePattern::StaticClass(), FGameplayTag()));

	LaunchTestPattern(*Pattern, Boss, {FVector2D(1000.f, 0.f)});
	TickTestTimers(*World, 0.f);
	World->TimeSeconds += 4.f;
	TickTestTimers(*World, 4.f);
	TActorIterator<AGeoTestReadableZone> const DuringWindUp(World);
	TestFalse(TEXT("During the wind-up: no zone exists to judge anything"), static_cast<bool>(DuringWindUp));

	float const LandingTime = World->TimeSeconds + 1.01f;
	World->TimeSeconds = LandingTime;
	TickTestTimers(*World, 1.01f);
	TActorIterator<AGeoTestReadableZone> const Landed(World);
	if (TestTrue(TEXT("After the landing: the zone exists"), static_cast<bool>(Landed)))
	{
		TestTrue(TEXT("After the landing: it judges"), Landed->GetJudgedCount() > 0);
		TestEqual(TEXT("Its growth clock starts at the landing, not at the firing"), Landed->GetGrowthStartServerTime(),
				  LandingTime, 0.01f);
	}

	static_cast<UPattern*>(Pattern)->EndPattern(true);
	GetMutableDefault<UGameDataSettings>()->DefaultZoneClass = ProjectZoneClass;
	DestroyTestWorld(World);
	return true;
}

#endif
