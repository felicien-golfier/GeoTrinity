// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/Hazard/SafeSpot/SafeSpot.h"
#include "AbilitySystem/Abilities/Pattern/Hazard/SafeSpot/SafeSpotMask.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Characters/PlayableCharacter.h"
#include "Engine/GameInstance.h"
#include "GameClasses/Match/GeoGameState.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
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
	FSafeSpotMask::Set(World, nullptr, Spots, FVector2D(1.f, 0.f));
	FSafeSpotMask::Clear(World, nullptr);
	DestroyTestWorld(World);

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
	auto const GetAimYaws = [](AActor* const Caster, EProjectileTarget const Target)
	{
		UGeoAbilitySystemComponent* const ASC = GeoASLib::GetGeoAscFromActor(Caster);
		FGameplayAbilitySpec const* Spec = ASC->FindAbilitySpecFromClass(UGeoTestSalveAbility::StaticClass());
		if (!Spec)
		{
			Spec = ASC->FindAbilitySpecFromHandle(
				ASC->GiveAbility(FGameplayAbilitySpec(UGeoTestSalveAbility::StaticClass(), 1)));
		}
		return CastChecked<UGeoTestSalveAbility>(Spec->GetPrimaryInstance())->GetAimYaws(Target);
	};

	TArray<float> const PlayerYaws = GetAimYaws(Arena->GetBoss(), EProjectileTarget::AllPlayers);
	if (TestEqual(TEXT("AllPlayers: one yaw per live player"), PlayerYaws.Num(), 2))
	{
		TestTrue(TEXT("At each player"), FMath::IsNearlyEqual(FMath::Abs(PlayerYaws[0] - PlayerYaws[1]), 180.f, 0.1f));
	}

	FGeoEnemySetup AllySetup;
	AllySetup.Team = ETeam::Player;
	AEnemyCharacter* const Ally = Arena->SpawnAdd(AEnemyCharacter::StaticClass(), FTransform(), &AllySetup);
	TestEqual(TEXT("Boss, no fight: none"), GetAimYaws(Ally, EProjectileTarget::Boss).Num(), 0);

	Arena->StartFight();
	TArray<float> const BossYaws = GetAimYaws(Ally, EProjectileTarget::Boss);
	if (TestEqual(TEXT("Boss, fighting: one yaw"), BossYaws.Num(), 1))
	{
		TestEqual(TEXT("At the fighting arena's boss"), BossYaws[0], 90.f, 0.1f);
	}

	Arena->GetBoss()->Death();
	TestEqual(TEXT("Boss dead: none"), GetAimYaws(Ally, EProjectileTarget::Boss).Num(), 0);

	DestroyTestWorld(World);
	return true;
}

#endif
