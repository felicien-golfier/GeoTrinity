// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Engine/CurveTable.h"
#include "Engine/GameInstance.h"
#include "GameClasses/Match/GeoGameMode.h"
#include "GameClasses/Match/GeoGameState.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Misc/AutomationTest.h"
#include "System/GeoSessionSubsystem.h"
#include "Tests/Gameplay/Fake/GeoTestDeployable.h"
#include "Tests/Gameplay/Fake/GeoTestWorld.h"
#include "Tool/GeoDifficulty.h"
#include "World/GeoWorldSettings.h"

#if WITH_DEV_AUTOMATION_TESTS

constexpr EAutomationTestFlags GeoTutorialTestFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

/** A begun listen-server world with a game mode, its session and a player start, ready for Login. */
struct FGeoLoginFixture
{
	explicit FGeoLoginFixture(bool const bSoloOnly)
	{
		World = CreateTestWorld();
		World->SetGameInstance(NewObject<UGameInstance>(GEngine));
		World->SpawnActor<APlayerStart>();
		GameMode = World->SpawnActor<AGeoGameMode>();
		GameMode->GameSession = World->SpawnActor<AGameSession>();
		GameMode->bSoloOnly = bSoloOnly;
	}

	~FGeoLoginFixture() { DestroyTestWorld(World); }

	/** Joins a player the way Login does for a joiner or a second local player: its controller spawned first, then
	 * InitNewPlayer, the controller destroyed on refusal. Error is empty on success. */
	APlayerController* Join(FString& Error) const
	{
		APlayerController* PlayerController = World->SpawnActor<APlayerController>();
		Error = GameMode->InitNewPlayer(PlayerController, FUniqueNetIdRepl(), FString(), FString());
		if (!Error.IsEmpty())
		{
			PlayerController->Destroy();
			PlayerController = nullptr;
		}

		return PlayerController;
	}

	UWorld* World = nullptr;
	AGeoGameMode* GameMode = nullptr;
};

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoTutorialSoloOnlyTest, "GeoTrinity.Tutorial.SoloOnly", GeoTutorialTestFlags)

bool FGeoTutorialSoloOnlyTest::RunTest(FString const& /*Parameters*/)
{
	{
		FGeoLoginFixture const Fixture(true);
		FString FirstError;
		TestNotNull(TEXT("Solo only: the first player is accepted"), Fixture.Join(FirstError));
		TestTrue(TEXT("Solo only: no error for the first player"), FirstError.IsEmpty());
		FString SecondError;
		TestNull(TEXT("Solo only: a second player is refused"), Fixture.Join(SecondError));
		TestFalse(TEXT("Solo only: the refusal carries an error"), SecondError.IsEmpty());
	}

	{
		FGeoLoginFixture const Fixture(false);
		FString Error;
		TestNotNull(TEXT("Not solo: the first player is accepted"), Fixture.Join(Error));
		TestNotNull(TEXT("Not solo: a second player is accepted"), Fixture.Join(Error));
		TestTrue(TEXT("Not solo: no error"), Error.IsEmpty());
	}

	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoTutorialTaggedStartTest, "GeoTrinity.Tutorial.TaggedStart", GeoTutorialTestFlags)

bool FGeoTutorialTaggedStartTest::RunTest(FString const& /*Parameters*/)
{
	FGeoLoginFixture const Fixture(true);
	APlayerStart* const JumpStart = Fixture.World->SpawnActor<APlayerStart>();
	JumpStart->PlayerStartTag = TEXT("Shoot");
	APlayerController* const PlayerController = Fixture.World->SpawnActor<APlayerController>();

	bool bTaggedChosen = false;
	for (int32 Attempt = 0; Attempt < 20; ++Attempt)
	{
		bTaggedChosen |= Fixture.GameMode->ChoosePlayerStart(PlayerController) == JumpStart;
	}

	TestFalse(TEXT("No portal: a tagged start is never chosen"), bTaggedChosen);
	TestTrue(TEXT("The portal still finds the tagged start"),
			 Fixture.GameMode->FindPlayerStart(PlayerController, TEXT("Shoot")) == JumpStart);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoTutorialSafeDifficultyTest, "GeoTrinity.Tutorial.SafeDifficulty",
								 GeoTutorialTestFlags)

bool FGeoTutorialSafeDifficultyTest::RunTest(FString const& /*Parameters*/)
{
	UClass* const TutorialModeClass =
		LoadClass<AGeoGameMode>(nullptr, TEXT("/Game/Game/BP_TutorialGameMode.BP_TutorialGameMode_C"));
	if (TestNotNull(TEXT("BP_TutorialGameMode loads"), TutorialModeClass))
	{
		TestTrue(TEXT("The tutorial game mode starts at Safe"),
				 GetDefault<AGeoGameMode>(TutorialModeClass)->StartDifficulty == EGeoDifficulty::Safe);
	}

	UWorld* const World = CreateTestWorld();
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.bDeferConstruction = true;
	AGeoGameMode* const GameMode = World->SpawnActor<AGeoGameMode>(SpawnParameters);
	GameMode->StartDifficulty = EGeoDifficulty::Safe;
	GameMode->FinishSpawning(FTransform::Identity);
	AGeoGameState const* const GameState = GameMode->GetGameState<AGeoGameState>();
	if (TestNotNull(TEXT("The game mode makes an AGeoGameState"), GameState))
	{
		TestTrue(TEXT("The game state takes the game mode's StartDifficulty"),
				 GameState->GetDifficulty() == EGeoDifficulty::Safe);
	}

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoTutorialMinibossSafeRowsTest, "GeoTrinity.Tutorial.MinibossSafeRows",
								 GeoTutorialTestFlags)

bool FGeoTutorialMinibossSafeRowsTest::RunTest(FString const& /*Parameters*/)
{
	UCurveTable const* const Table =
		LoadObject<UCurveTable>(nullptr, TEXT("/Game/AbilitySystem/Data/CurveTables/CT_Tutorial.CT_Tutorial"));
	if (TestNotNull(TEXT("CT_Tutorial loads"), Table))
	{
		struct FRowValues
		{
			FName Row;
			float Safe;
			float Original;
		};

		for (FRowValues const& Values :
			 {FRowValues{TEXT("MinibossProjectileDamage"), 5.f, 8.f},
			  FRowValues{TEXT("AllyProjectileDamage"), 15.f, 8.f}, FRowValues{TEXT("AllyDummyHealth"), 50.f, 200.f},
			  FRowValues{TEXT("DummyZoneStartRadius"), 10.f, 150.f},
			  FRowValues{TEXT("WaveAddLaneRadius"), 127.f, 190.f}})
		{
			FRealCurve const* const Curve = Table->FindCurveUnchecked(Values.Row);
			if (TestNotNull(*Values.Row.ToString(), Curve))
			{
				TestEqual(*FString::Printf(TEXT("%s at Safe"), *Values.Row.ToString()),
						  Curve->Eval(static_cast<float>(GeoDifficultyMask::Safe)), Values.Safe);
				TestEqual(*FString::Printf(TEXT("%s at Original"), *Values.Row.ToString()),
						  Curve->Eval(static_cast<float>(GeoDifficultyMask::Original)), Values.Original);
			}
		}
	}

	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoSessionListenUrlTest, "GeoTrinity.Session.ListenUrl", GeoTutorialTestFlags)

bool FGeoSessionListenUrlTest::RunTest(FString const& /*Parameters*/)
{
	TestEqual(TEXT("No portal"), UGeoSessionSubsystem::MakeListenUrl(TEXT("/Game/Maps/Tutorial"), FString()),
			  FString(TEXT("/Game/Maps/Tutorial?listen")));
	TestEqual(TEXT("With a portal"),
			  UGeoSessionSubsystem::MakeListenUrl(TEXT("/Game/Maps/DraftMap"), TEXT("TrainingRoom")),
			  FString(TEXT("/Game/Maps/DraftMap?listen#TrainingRoom")));
	return true;
}

/** A begun world whose settings lock the first startup ability of the Circle, and a pawn with an ASC to grant to. */
struct FGeoLockedAbilityFixture
{
	explicit FGeoLockedAbilityFixture(FAutomationTestBase& Test)
	{
		World = CreateTestWorld();
		UAbilityInfo const* const AbilityInfo = GeoASLib::GetAbilityInfo();
		AGeoWorldSettings* const WorldSettings = Cast<AGeoWorldSettings>(World->GetWorldSettings());
		if (Test.TestNotNull(TEXT("Ability info"), AbilityInfo)
			&& Test.TestNotNull(TEXT("Geo world settings"), WorldSettings))
		{
			for (FPlayersGameplayAbilityInfo const& Info : AbilityInfo->GetAbilitiesForClass(EPlayerClass::Circle))
			{
				if (Info.bGiveAtStartup && Info.AbilityClass && Info.InputTag.IsValid())
				{
					if (LockedTag.IsValid())
					{
						FreeTags.Add(Info.AbilityTag);
					}
					else
					{
						LockedTag = Info.AbilityTag;
					}
				}
			}
			WorldSettings->LockedAbilityTags.AddTag(LockedTag);
		}
		ASC = Cast<UGeoAbilitySystemComponent>(World->SpawnActor<AGeoTestDeployer>()->GetAbilitySystemComponent());
	}

	~FGeoLockedAbilityFixture() { DestroyTestWorld(World); }

	FGameplayAbilitySpec const* FindSpec(FGameplayTag const AbilityTag) const
	{
		return ASC->GetActivatableAbilities().FindByPredicate(
			[AbilityTag](FGameplayAbilitySpec const& Spec)
			{
				return GeoASLib::GetAbilityTagFromSpec(Spec) == AbilityTag;
			});
	}

	UWorld* World = nullptr;
	UGeoAbilitySystemComponent* ASC = nullptr;
	FGameplayTag LockedTag;
	TArray<FGameplayTag> FreeTags;
};

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoTutorialLockedAbilitiesTest, "GeoTrinity.Tutorial.LockedAbilities",
								 GeoTutorialTestFlags)

bool FGeoTutorialLockedAbilitiesTest::RunTest(FString const& /*Parameters*/)
{
	FGeoLockedAbilityFixture const Fixture(*this);
	if (TestTrue(TEXT("The Circle has a locked and a free startup ability"),
				 Fixture.LockedTag.IsValid() && !Fixture.FreeTags.IsEmpty()))
	{
		Fixture.ASC->GiveStartupAbilities(EPlayerClass::Circle);
		TestNull(TEXT("The locked ability is not granted"), Fixture.FindSpec(Fixture.LockedTag));
		for (FGameplayTag const& FreeTag : Fixture.FreeTags)
		{
			TestNotNull(*FString::Printf(TEXT("%s is granted"), *FreeTag.ToString()), Fixture.FindSpec(FreeTag));
		}
	}

	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoTutorialUnlockAbilityTest, "GeoTrinity.Tutorial.UnlockAbility",
								 GeoTutorialTestFlags)

bool FGeoTutorialUnlockAbilityTest::RunTest(FString const& /*Parameters*/)
{
	FGeoLockedAbilityFixture const Fixture(*this);
	if (TestTrue(TEXT("The Circle has a locked startup ability"), Fixture.LockedTag.IsValid()))
	{
		Fixture.ASC->GiveStartupAbilities(EPlayerClass::Circle);
		int32 const GrantedBefore = Fixture.ASC->GetActivatableAbilities().Num();
		int32 SetChanges = 0;
		FDelegateHandle const Handle = Fixture.ASC->OnAbilitySetChanged.AddLambda(
			[&SetChanges]()
			{
				++SetChanges;
			});
		TestTrue(TEXT("The unlock grants"), Fixture.ASC->GiveClassAbility(EPlayerClass::Circle, Fixture.LockedTag));
		TestEqual(TEXT("The grant fires OnAbilitySetChanged once"), SetChanges, 1);
		FGameplayAbilitySpec const* const Spec = Fixture.FindSpec(Fixture.LockedTag);
		TArray<FPlayersGameplayAbilityInfo> const CircleInfos =
			GeoASLib::GetAbilityInfo()->GetAbilitiesForClass(EPlayerClass::Circle);
		FPlayersGameplayAbilityInfo const* const Info = CircleInfos.FindByPredicate(
			[&Fixture](FPlayersGameplayAbilityInfo const& Candidate)
			{
				return Candidate.AbilityTag == Fixture.LockedTag;
			});
		if (TestNotNull(TEXT("The ability is granted"), Spec) && Info)
		{
			TestTrue(TEXT("Bound to its input tag"), Spec->GetDynamicSpecSourceTags().HasTagExact(Info->InputTag));
		}

		TestFalse(TEXT("A second unlock grants nothing"),
				  Fixture.ASC->GiveClassAbility(EPlayerClass::Circle, Fixture.LockedTag));
		TestEqual(TEXT("A refused unlock fires nothing"), SetChanges, 1);
		Fixture.ASC->OnAbilitySetChanged.Remove(Handle);
		TestEqual(TEXT("One more ability in all"), Fixture.ASC->GetActivatableAbilities().Num(), GrantedBefore + 1);
	}

	return true;
}

#endif
