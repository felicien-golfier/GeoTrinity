// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Engine/GameInstance.h"
#include "GameClasses/Match/GeoGameMode.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Misc/AutomationTest.h"
#include "System/GeoSessionSubsystem.h"
#include "Tests/GeoTestDeployable.h"
#include "Tests/GeoTestWorld.h"
#include "World/GeoWorldSettings.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

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
